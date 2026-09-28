#!/bin/sh

#
# Edit this with your local proxy settings, chmod +x,
# and you're good to go. :-)
#

proxy="socks5h://use:tor@127.0.0.1:9050"

mkdir -p vendor

while read ofile url; do
	curl -A "" -x "${proxy}" \
		-D "vendor/${ofile}.head" \
		-o "vendor/${ofile}" \
		"${url}"
done < dl.txt
