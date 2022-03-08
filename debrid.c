#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <regex.h>

#define PROXY_FILE "SOCKS5.txt"
#define PROXY_WARN 43200 
#define PROXY_URL "https://raw.githubusercontent.com/manuGMG/proxy-365/main/SOCKS5.txt"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64)\
	AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36"

CURL *curl;
CURLcode res;
struct Doc {
	char *content;
	size_t size;

};

const int vlinks_size = 11;
const char valid_links[11][16] = {
	"1fichier.com",
	"alterupload.com",
	"cjoint.net",
	"desfichiers.com",
	"dfichiers.com",
	"megadl.fr",
	"mesfichiers.org",
	"piecejointe.net",
	"pjointe.com",
	"tenvoi.com",
	"dl4free.com"
};

static size_t write_cb(void *content, size_t size, size_t nmemb, struct Doc *d) {
	size_t new_size = d->size + size*nmemb;
	d->content = realloc(d->content, new_size+1);

	if (d->content == NULL) {
		fprintf(stderr, "Failed to realloc memory\n");
		exit(EXIT_FAILURE);
	}
	
	memcpy(d->content + d->size, content, size*nmemb);
	d->content[new_size] = '\0';
	d->size = new_size;

	return size*nmemb;
}

void get_proxy() {
	int line_count;
	int rand_line;
	int i;
	char buf[32];

	// Open PROXY_FILE in read mode
	FILE *fp = fopen(PROXY_FILE, "r");

	if (fp == NULL) {
		printf("Failed to open %s\n", PROXY_FILE);
	}

	// Get line count
	for (line_count = 0; fgets(buf, sizeof(buf), fp); line_count++)
		;

	rewind(fp);	

	// Get random line (rand. proxy)
	srand(time(NULL)); // seeding with time should be good enough
	rand_line = rand() % (line_count+1);

	for (i = 0; i < rand_line; i++)
		fgets(buf, sizeof(buf), fp);

	// Remove trailing new line
	buf[strcspn(buf, "\n")] = 0;	
	fclose(fp);

	// Set proxy opts
	curl_easy_setopt(curl, CURLOPT_PROXY, buf);
	curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
}

int validate(char *url) {
	int valid = 0;
	char *regex_string;

	//check through the list of recognized domains for a substring match in the url
	for (int i = 0; i < vlinks_size; i++) {
		if (strstr(url, valid_links[i]) != NULL) {
			asprintf(&regex_string, "https://%s/\\?[a-zA-Z0-9]{20}$", valid_links[i]);
			valid=1;
			break;
		}
	}
	
	//return false if url domain domain is not in array of recognized domains
	if (!valid) {
		return 0;
	}

	regex_t compiledRegex;

	//security check in case regex suddenly stops compiling one day
	if(regcomp(&compiledRegex, regex_string, REG_EXTENDED)) {
		printf("Could Not Compile Regular Expression\n");
		return 1;
	}

	//return true if download url iis valid
	if(regexec(&compiledRegex, url, 0, NULL, 0) == 0) {
		return 1;
	}

	//return false if download url is invaild
	return 0;
}



void debrid(char *url) {
	int valid = 0;
	int i = 0;
	int position;
	char *pch;

	struct Doc document;
	document.content = malloc(1);
	document.content[0] = '\0';
	document.size = 0;

	// Validate URL
	if (!validate(url)){
			printf("Invalid URL\n");
			return;
	}

	// Loop attempts
	while (1) {
		if (curl) {
			// Get proxy and set opts
			get_proxy();

			// POST request to get to download button
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "dl_no_ssl=on&dlinline=on");
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &document);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
			res = curl_easy_perform(curl);

			if (res != CURLE_OK) {
				fprintf(stderr, "[!] %s\n", curl_easy_strerror(res));
				continue;
			} else {
				// Honestly, fuck xml libs
				// its just a link
				pch = strstr(document.content, "?inline");

				if (pch != NULL) {
					for (position = 0; pch[position] != '"'; position--)
						;
					printf("[*] Parsed link: %.*s\n", -position+6, pch+position+1);
				} else {
					printf("[!] Failed to parse link\n");
					continue;
				}
			}

			free(document.content);
			curl_easy_cleanup(curl);
			break;
		}
	}	
}

int main(int argc, char *argv[]) {
	int i;

	// Using stat to get the last modified date of proxy file
	// and to check whether it exists or not.
	struct stat proxy_attr;

	// Validate arguments
	if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printf("USAGE: %s {url}\n", argv[0]);
		return 1;
	}
	
	// Initialize curl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	// Check if proxy file exists, else download from
	// PROXY_URL
	if (stat(PROXY_FILE, &proxy_attr) == 0) {
		// Check if proxy is older than PROXY_WARN,
		// warn if so
		if (difftime(time(NULL), proxy_attr.st_mtime) > PROXY_WARN) {
			printf("[!] %s is outdated, consider replacing it.\n", PROXY_FILE);
		}
	} else {
		// Download PROXY_FILE from PROXY_URL
		if (curl) {
			FILE *fp = fopen(PROXY_FILE, "w");
			
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_URL, PROXY_URL);

			res = curl_easy_perform(curl);

			if (res != CURLE_OK) {
				fprintf(stderr, "FAILED TO DOWNLOAD PROXIES: %s\n", curl_easy_strerror(res));
				exit(EXIT_FAILURE);
			}

			// Cleanup
			curl_easy_reset(curl);
			fclose(fp);
		}
	}

	// Debrid URLs 
	for (i = 1; i < argc; i++) {
		debrid(argv[i]);
	}

	curl_global_cleanup();
	return 0;
}
