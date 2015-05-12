# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) open "sample.txt" once
(test) open "sample.txt" again
(test) end
open-twice: exit(0)
EOF
pass;
