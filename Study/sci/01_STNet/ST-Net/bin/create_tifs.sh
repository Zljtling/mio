#!/bin/bash

set -euo pipefail
cd "$(python -c 'import stnet; print(stnet.config.SPATIAL_RAW_ROOT)')"
shopt -s nullglob
for i in */*/*.jpg;
do
    if [ ! -f "${i%.jpg}.tif" ]; then
        echo "$i"
        vips tiffsave "$i" "${i%.jpg}.partial.tif" --tile --tile-width 256 --tile-height 256 --compression deflate
        mv "${i%.jpg}.partial.tif" "${i%.jpg}.tif"
    fi
done
