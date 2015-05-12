# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(child-simple) run
child-simple: exit(81)
(test) wait(exec()) = 81
(test) end
wait-simple: exit(0)
EOF
pass;
