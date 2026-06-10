program bad (input, output);
begin
  if x then
    y := 1
  { missing 'else' causes IF-THEN-ELSE rule to fail —
    but the real syntax error here is: x is used without
    a prior 'var' declaration, which is a missing required
    grammar production for declarations }
end.
