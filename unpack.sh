#!/bin/sh

set -e

tarball="$1"
out="_katex_"

if [ ! -f "${tarball}" ]; then
	echo "ENOENT: ${tarball}" >&2
	exit 1
fi

mkdir -p "${out}/fonts"

tar -xf "${tarball}" -C "${out}" --strip-components=1 "katex/katex.min.css"

tar -xf "${tarball}" -C "${out}" --strip-components=1 "katex/katex.js"
tar -xf "${tarball}" -C "${out}" --strip-components=2 "katex/contrib/mhchem.js"

tar -xf "${tarball}" -C "${out}/fonts" --strip-components=2 \
	--wildcards \
	"katex/fonts/*.woff2"
