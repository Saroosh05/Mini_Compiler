program demo (input, output);
var i, x : integer;
     arr : array [1..10] of integer;

function add(a : integer) : integer;
begin
  add := a + 1
end;

begin
  i := 1;
  if i <= 10 then
    x := add(i)
  else
    x := 0;
  while i < 10 do
    i := i + 1
end.
