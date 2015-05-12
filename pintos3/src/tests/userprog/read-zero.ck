# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) open "sample.txt"
(test) end
read-zero: exit(0)
EOF
pass;
