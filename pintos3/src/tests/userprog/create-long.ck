# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) create("x..."): 0
(test) end
create-long: exit(0)
EOF
pass;
