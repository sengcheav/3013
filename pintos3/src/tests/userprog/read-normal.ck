# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) open "sample.txt" for verification
(test) verified contents of "sample.txt"
(test) close "sample.txt"
(test) end
read-normal: exit(0)
EOF
pass;
