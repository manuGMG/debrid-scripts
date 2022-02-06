#!/usr/bin/env bash
# debrid-scripts

PROXY_FILE="SOCKS5.txt"
PROXY_WARN=$(( $(date +%s) - 43200 )) # 43200s -> 12h
PROXY_URL="https://raw.githubusercontent.com/manuGMG/proxy-365/main/SOCKS5.txt"

# Colors
COLOR_ERR=$(tput bold && tput setaf 1) # Bold red 
COLOR_INF=$(tput bold && tput setaf 6) # Bold cyan
COLOR_RES=$(tput sgr 0)                # Reset

# Logging functions
err() { printf "\33[2K\r$COLOR_ERR[!]$COLOR_RES $1\n" ; }
info() { printf "\33[2K\r$COLOR_INF[*]$COLOR_RES $1" ; }

# Debrid function 
#	Validate URL
# 	-> Get PROXY (from PROXY_FILE)
#	-> POST to [link] using PROXY, get to download button
#	-> Parse HTML, paste link on console and opens it on browser
debrid() {
	# (Lazily) Validate URL
	if [[ "$1" != *"http"* ]]
	then
		err "Invalid URL" && return 1
	elif [[ "$1" == *"/dir/"* ]]
	then
		err "Directories are not supported yet" && return 1
	fi
	
	# Loop attempts
	while [ 1 ]
	do
		# Grab proxies
		PROXY=$(shuf -n 1 $PROXY_FILE)
		info "Using proxy: $PROXY"

		# Parse direct link 
		DIRECT=$(curl -m 20 -s --socks5 $PROXY -X POST -d "dl_no_ssl=on&dlinline=on" $1 |
			hxnormalize -x |
			hxselect -i "a.ok" |
			hxwls)

		# Download file
		if [[ -z "$DIRECT" ]]
		then
			err "Could not parse direct link"
		else
			info "Parsed direct link: $DIRECT\n"
			if [ "$(uname)" == "Linux" ]; then
				xdg-open $DIRECT
			break
		fi
	done
}

# Validate arguments, initial checks and debrid.
if [[ -z "$1" || "$1" = "-h" || "$1" = "--help" ]]
then
	err "Usage: $0 [url]"
else
	# Check dependencies (curl, html-xml-utils)
	dependencies=(curl hxnormalize)
	for dep in ${dependencies[@]}; do
		if ! command -v $dep &> /dev/null
		then
			err "$dep is not installed" && exit
		fi
	done

	# Check if proxy file exists
	if [[ -f "$PROXY_FILE" ]]
	then
		# Check if proxy file is older than PROXY_WARN (secs)
		if [[ $(date -r $PROXY_FILE +%s) -lt "$PROXY_WARN" ]]
		then
			err "$PROXY_FILE is outdated, consider replacing it!"
		fi
	else
		# Get proxies from proxy-365 repo
		info "Grabbing proxies\n"
		curl -s -o $PROXY_FILE $PROXY_URL || exit
	fi

	for link in "$@"; do
		debrid $link 
	done
fi
