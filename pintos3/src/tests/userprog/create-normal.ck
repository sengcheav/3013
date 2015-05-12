# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) create quux.dat
(test) end
create-normal: exit(0)
EOF
pass;
