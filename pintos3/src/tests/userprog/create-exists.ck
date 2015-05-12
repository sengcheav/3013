# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) create quux.dat
(test) create warble.dat
(test) try to re-create quux.dat
(test) create baffle.dat
(test) try to re-create quux.dat
(test) end
create-exists: exit(0)
EOF
pass;
