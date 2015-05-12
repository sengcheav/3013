# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF', <<'EOF']);
(test) begin
(test) create(""): 0
(test) end
create-empty: exit(0)
EOF
(test) begin
create-empty: exit(-1)
EOF
pass;
