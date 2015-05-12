# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(child-simple) run
child-simple: exit(81)
(test) end
exec-once: exit(0)
EOF
pass;
