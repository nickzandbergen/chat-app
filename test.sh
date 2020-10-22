echo -n -e a\\x00\\x03foo | nc 127.0.0.1 $1
echo -n -e a\\x00\\x03bar | nc 127.0.0.1 $1
echo -n -e a\\x00\\x03baz | nc 127.0.0.1 $1

echo -n -e l\\x00\\x03baz | nc 127.0.0.1 $1