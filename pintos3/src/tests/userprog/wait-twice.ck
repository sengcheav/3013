# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(child-simple) run
child-simple: exit(81)
(test) wait(exec()) = 81
(test) wait(exec()) = -1
(test) end
wait-twice: exit(0)
EOF
pass;
