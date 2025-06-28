# Multithreaded Web Crawler in C++

This project implements a **multithreaded web crawler** using C++. It downloads web pages, extracts hyperlinks, and recursively processes them up to a specified depth. The program is designed to efficiently handle multiple URLs in parallel using multithreading.

---

## **Features**

- **HTML Downloading**: Uses `libcurl` to fetch and save web pages locally.
- **Hyperlink Extraction**: Extracts links from HTML files using regular expressions.
- **Link Validation**: Validates hyperlinks to ensure they are well-formed.
- **Recursive Crawling**: Processes web pages up to a depth of 4.
- **Multithreading**: Handles multiple URLs concurrently using C++ threads.
- **Performance Metrics**: Reports time taken for processing each web page.
- **Rate Limiting**: Introduces delays to prevent server overload.

---

## **Requirements**

- C++17 or later
- `libcurl` library installed

---

## **Installation**

1. Clone the repository:
   ```bash
   git clone <your-repo-url>
   cd <your-repo-folder>
   ```

2. Install `libcurl`:
   ```bash
   sudo apt-get update
   sudo apt-get install libcurl4-openssl-dev
   ```

3. Compile the program:
   ```bash
   g++ -std=c++17 -pthread -lcurl -o web_crawler main.cpp
   ```

---

## **Usage**

1. Run the program:
   ```bash
   ./web_crawler
   ```

2. Example Output:
   ```
   Time taken to generate thread 1 is: 2.3 seconds
   Thread_id: 1    Link: https://www.iitism.ac.in/

   Time taken to generate thread 2 is: 1.7 seconds
   Thread_id: 2    Link: https://en.wikipedia.org/wiki/Main_page

   Time taken to generate thread 3 is: 2.1 seconds
   Thread_id: 3    Link: https://codeforces.com/
   ```

---

## **Code Overview**

### `get_page`:
Fetches and saves the HTML content of a URL to a file using `libcurl`.

### `extract_hyperlinks`:
Extracts all hyperlinks from the saved HTML file using regular expressions.

### `cleanUp`:
Validates and cleans up extracted links.

### `dfs_crawler`:
Recursively crawls web pages to extract and process hyperlinks. Includes depth control and multithreading support.

### `main`:
Initializes threads for crawling multiple URLs concurrently.

---

## **Project Structure**
```
|-- main.cpp          # Main program file
|-- README.md         # Project documentation
```

---

## **To-Do**

- Add error handling for network issues.
- Store extracted links in a file or database for further processing.
- Introduce dynamic depth control via user input.
- Improve rate-limiting logic.

---

## **Contributing**

Contributions are welcome! If you have ideas for improvements, please submit a pull request or open an issue.

---

## **Contact**

If you have any questions or suggestions, feel free to reach out!

