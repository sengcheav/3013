# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) open "sample.txt"
(test) end
write-zero: exit(0)
EOF
pass;
