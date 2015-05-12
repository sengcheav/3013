# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(test) begin
(test) end
open-missing: exit(0)
EOF
pass;
