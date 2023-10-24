#!/bin/bash

# this script runs smog and observes the dirty pages using page-types
# assuming that page-types is in your PATH

cat > /tmp/smogfile.yml << EOF
---
buffers:
  - size: 1GiB
    slices:
      - range: 0...<100%
threads:
  - buffer: 0:0
    kernel: dirty pages
    delay: 100000
EOF

./smog -v /tmp/smogfile.yml &> /tmp/smog.log &
smog_pid=$!

echo "$smog_pid"

while ! grep "Creating 1 SMOG buffers" /tmp/smog.log; do sleep 1; done

buf="$(grep -a -A2 "Creating 1 SMOG buffers" /tmp/smog.log | tail -n1 | \
  sed 's/ *At \(0x[0-9a-f]*\) \.\.\. \(0x[0-9a-f]*\)/\1,\2/')"

echo "$buf"
from="$(echo "$buf" | cut -d',' -f1)"
unto="$(echo "$buf" | cut -d',' -f2)"

range="$(( from / 4096 )),$(( (unto + 1) / 4096 ))"
echo "$range"

echo "4" | sudo tee /proc/"$smog_pid"/clear_refs

watch -n 0.1 sudo page-types -r -p "$smog_pid" -a "$range"
