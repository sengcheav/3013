# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) open "sample.txt"
(test) close "sample.txt"
(test) end
close-normal: exit(0)
EOF
pass;
