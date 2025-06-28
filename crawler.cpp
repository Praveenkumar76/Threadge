#include <bits/stdc++.h>
#include <unordered_map>
#include <curl/curl.h> //for downloading webpages
#include <stdio.h>
#include <regex> //to handle regular expressions
#include <thread>
#include <mutex> //to avoid race conditions
#include <chrono> //time-related functions
using namespace std;

mutex m;

//download html in file
void get_page(const char* url, const char* file_name)
{
  CURL* easyhandle = curl_easy_init(); // initialize the curl Object

  curl_easy_setopt( easyhandle, CURLOPT_URL, url ) ; //sets the url which will be used for http request
  FILE* file = fopen(file_name, "w");//open file in write mode
  
  curl_easy_setopt( easyhandle, CURLOPT_WRITEDATA, file) ;//specifies file where response data will be written
  curl_easy_perform( easyhandle ); //performs actual http request 
  curl_easy_cleanup( easyhandle );//cleans up easyhandle object and frees any associated resource
  fclose(file);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------

//extract links from file
vector<string> extract_hyperlinks(string html_file_name )
{
    string html; // here we will store the html from file
    ifstream read;
    read.open(html_file_name); 
    while(!read.eof())
    {
      if(read.eof()) break; 
      char ch;
      read.get(ch);
      html.push_back(ch); 
    }
    read.close();
    static const regex hl_regex( "<a href=\"(.*?)\">", regex_constants::icase ); // initializing a regular expression object h1_regex that finds all hyperlinks in html file,icase mean not case sensitive
    vector<string> links; // it will store all the links read from file
    //below line will store all found links in vector
    copy(sregex_token_iterator(html.begin(), html.end(), hl_regex, 1),sregex_token_iterator(),back_inserter(links));
    return links;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//validate the links
void cleanUp(vector<string>&all_links )
{
  vector<string>final_links;// it will store all the valid links
  for(int i=0;i<all_links.size();i++) // cleaner
  {
    string one_link = all_links[i];//pick one link
    string cleaned_link="";//it will store cleaned link
    for(int j=0;j<one_link.length();j++)
    {
      if(one_link[j] == ' ' || one_link[j] == 34 ) break; // stops if it will find empty space or double quotation
      cleaned_link.push_back(one_link[j]); // concatenating character by character to string
    }
    //after getting useful part now validate the link,if url is good to go then add it to final_vector
    if (regex_match(cleaned_link,regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)") )) {
      final_links.push_back(cleaned_link);
    }
  } 
  all_links.clear(); // clear original vector
  all_links = final_links; // replace it with final links vector
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unordered_map<string,bool>visited; // used while dfs to identify url is already visited or not :TC O(1) 

void dfs_crawler(const char* url,const char* file_path,int depth,int bot_id)
{
  if(depth==4 || visited[url]) return; // if depth=4 or url is already visited
  string s = url;//store url in string s
  visited[s]=1; // mark current url as visited
  
  m.lock();//using it for synchronization
  clock_t startTime = clock();
  get_page(url, file_path); // get html of current url

  vector<string> allLinksData  =  extract_hyperlinks(file_path); // get all links
  cleanUp(allLinksData);//clean all the links
  m.unlock();//unlock the mutex lock

  
  clock_t stopTime = clock();
  float elapsedTime = (float)(stopTime-startTime)/CLOCKS_PER_SEC;
  cout<<"Time taken to generate thread "<<bot_id<<" is: "<<elapsedTime<<" seconds"<<endl;
  cout << "Thread_id: "<< bot_id <<"\tLink: "<< url << endl<<endl;//print the url and thread id
    
  for(string i : allLinksData)//dfs
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));//sleep for 10ms to get the , server overload n ho
    const char *c = i.c_str();
    
    if(visited[i]!=1){ // if url is not visited then call the function

      dfs_crawler(c,file_path,depth+1,bot_id);
    }
  }

 
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main()
{

  const char *filename = "1.txt"; // file where html will be stored
  thread t1(dfs_crawler,"https://www.iitism.ac.in/",filename,0,1);
  cout<<endl;
  thread t2(dfs_crawler,"https://en.wikipedia.org/wiki/Main_page",filename,0,2);
  cout<<endl;
  thread t3(dfs_crawler,"https://codeforces.com/",filename,0,3);
  cout<<endl;
  t1.join(); //function to make the main thread wait until the newly created thread has finished its execution,all the threads have completed their execution before the program terminates.
  t2.join();
  t3.join();

  return 0;
}