# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) create "test.txt"
(test) open "test.txt"
(test) end
write-normal: exit(0)
EOF
pass;
