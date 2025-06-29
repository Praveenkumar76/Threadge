#include <iostream>
#include <fstream>
#include <queue>
#include <regex>
#include <curl/curl.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <iterator>
#include <condition_variable>

using namespace std;
using namespace chrono;

class Scrap {
    public:
        unordered_map<string, bool> visited;
        mutex mtx;
        condition_variable cv;
        queue<pair<string, int>> urlQueue;
        bool done = false;
        int maxDepth;
        string keyword;

    bool get_link(const string& Url, const char* file_name) {
        CURL* curl = curl_easy_init();
        if(!curl) {
            cerr << "CURL initialization failed" << endl;
            return false;
        }
        FILE* file = fopen(file_name,"w");
        if(!file) {
            cerr << "Error while opening the file" << endl;
            return false;
        }
        curl_easy_setopt(curl,CURLOPT_URL,Url.c_str());
        curl_easy_setopt(curl,CURLOPT_WRITEDATA,file);
        curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(curl,CURLOPT_TIMEOUT,10L);
        curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        fclose(file);
        curl_easy_cleanup(curl);
        if(res != CURLE_OK || http_code != 200) {
            remove(file_name);  
            cerr << "Download failed (" << http_code << "): " << curl_easy_strerror(res) << endl;
            return false;
        }
        return true;
    }

    vector<string> extractlinks(const char* filename, const string& keyword)
    {
        ifstream read(filename);
        if (!read) {
            cerr << "Unable to read" << endl;
            return {};
        }
        string html(
            (istreambuf_iterator<char>(read)),
            istreambuf_iterator<char>()
        );
        static const regex r1("<a\\s+href\\s*=\\s*\"(.*?)\"", regex_constants::icase);
        vector<string> links;
        for (sregex_token_iterator it(html.begin(), html.end(), r1, 1);
            it != sregex_token_iterator();
            ++it)
        {
            string link = *it;
            if (link.find(keyword) != string::npos) {
                links.push_back(link);
            }
        }
        return links;
    }

    void dfs_crawler(const string& Url, const char* filepath, int depth, int bot_id, const string& keyword, const string& output_file) {
        {
            lock_guard<mutex> lock(mtx);
            if (visited[Url]) return;
            visited[Url] = true;
        }
        if (depth == maxDepth) return;
        auto start = chrono::high_resolution_clock::now();
        bool success = false;
        for (int attempt = 0; attempt < 3; attempt++) {
            if (get_link(Url, filepath)) {
                success = true;
                break;
            }
            this_thread::sleep_for(seconds(1));
        }
        if (!success) {
            lock_guard<mutex> lock(mtx);
            cout << "[Thread " << bot_id << "] Failed after retries: " << Url << endl;
            return;
        }
        ifstream read(filepath);
        string html(
            (istreambuf_iterator<char>(read)),
            istreambuf_iterator<char>()
        );
        if (html.empty()) {
            lock_guard<mutex> lock(mtx);
            cout << "[Thread " << bot_id << "] Empty page: " << Url << endl;
            return;
        }
        if (contains_keyword(html, keyword)) {
            auto snippets = extract_text_snippets(html);
            save_match(Url, snippets, output_file);
        }
        vector<string> allLinksData = extractlinks(filepath, keyword);
        cleanup(allLinksData, Url);
        auto end = high_resolution_clock::now();
        duration<double> elapsed = end - start;
        {
            lock_guard<mutex> lock(mtx);
            cout << "Time taken to generate thread " << bot_id
                << " is: " << elapsed.count() << " seconds" << endl;
            cout << "Thread_id: " << bot_id << "\tLink: " << Url << endl << endl;
        }
        for (const string& i : allLinksData) {
            {
                lock_guard<mutex> lock(mtx);
                urlQueue.push({i, depth + 1});
                cv.notify_one();
            }
            this_thread::sleep_for(milliseconds(100 + rand() % 200));
        }
    }

    bool contains_keyword(const string& html, const string& keyword) {
        return html.find(keyword) != string::npos;
    }

    void save_match(const string& url, const vector<string>& snippets, const string& output_file) {
        lock_guard<mutex> lock(mtx);
        ofstream out(output_file, ios::app);
        if (out) {
            out << "URL: " << url << endl;
            for (const auto& snip : snippets) {
                out << "Snippet: " << snip << endl;
            }
            out << "------------------------------------" << endl;
        }
    }
    
    void cleanup(vector<string>& all_links, const string& current_page_url) {
        vector<string> final_links;
        unordered_set<string> seen;
        regex url_regex("((http|https)://)(www\\.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,24}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
        for (auto& one_link : all_links) {
            string cleaned_link;
            size_t pos = one_link.find_first_of(" \"");
            if (pos != string::npos) {
                cleaned_link = one_link.substr(0, pos);
            } else {
                cleaned_link = one_link;
            }
            cleaned_link = make_absolute(current_page_url, cleaned_link);
            if (regex_match(cleaned_link, url_regex)) {
                if (seen.find(cleaned_link) == seen.end()) {
                    final_links.push_back(cleaned_link);
                    seen.insert(cleaned_link);
                }
            }
        }
        all_links = final_links;
    }

    string make_absolute(const string& base_url, const string& link) {
        if (link.empty()) return "";
        if (link.find("http://") == 0 || link.find("https://") == 0) {
            return link;
        }
        regex domain_regex(R"(^(https?://[^/]+))");
        smatch match;

        if (link[0] == '/') {
            if (regex_search(base_url, match, domain_regex)) {
                return match[1].str() + link;
            }
        } else {
            auto pos = base_url.find_last_of('/');
            if (pos != string::npos) {
                string base_path = base_url.substr(0, pos + 1);
                return base_path + link;
            }
        }
        if (regex_search(base_url, match, domain_regex)) {
            return match[1].str() + "/" + link;
        }
        return link;
    }

    void worker(int bot_id, const string& keyword, const string& output_file, const char* filepath) {
        while (true) {
            pair<string, int> task;

            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [&]() {
                    return !urlQueue.empty() || done;
                });

                if (done && urlQueue.empty()) {
                    return;
                }

                task = urlQueue.front();
                urlQueue.pop();
            }
            dfs_crawler(task.first, filepath, task.second, bot_id, keyword, output_file);
        }
    }

    vector<string> extract_text_snippets(const string& html) {
        vector<string> snippets;
        vector<regex> tags = {
            regex("<title>(.*?)</title>", regex_constants::icase),
            regex("<h1[^>]*>(.*?)</h1>", regex_constants::icase),
            regex("<h2[^>]*>(.*?)</h2>", regex_constants::icase),
            regex("<p[^>]*>(.*?)</p>", regex_constants::icase)
        };
        for (const auto& tag : tags) {
            for (sregex_iterator it(html.begin(), html.end(), tag), end; it != end; ++it) {
                string snippet = it->str(1);
                if (!snippet.empty()) {
                    snippets.push_back(snippet);
                }
            }
        }
        return snippets;
    }
};

int main() {
    srand(time(nullptr));

    Scrap scraper;

    cout << "Enter initial URL: ";
    string Url;
    getline(cin >> ws, Url);

    cout << "Enter keyword to extract: ";
    string key;
    getline(cin >> ws, key);
    scraper.keyword = key;

    cout << "Enter output file name: ";
    string output_file;
    getline(cin >> ws, output_file);

    cout << "Enter max depth: ";
    cin >> scraper.maxDepth;

    scraper.urlQueue.push({Url, 0}); 
    scraper.cv.notify_all();

    const int num_threads = 5;

    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(
            &Scrap::worker, &scraper, 
            i + 1, key, output_file, "temp.html"
        );
    }

    {
        unique_lock<mutex> lock(scraper.mtx);
        scraper.done = true;
    }
    scraper.cv.notify_all();

    for (auto& t : threads) {
        t.join();
    }

    cout << "Scraping finished." << endl;

    return 0;
}