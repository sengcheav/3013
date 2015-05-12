# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(args-none) begin
(args-none) end
args-none: exit(0)
EOF
pass;
