# REDIS DB

## MAIN FILE
variables : argc -> arguments


## REDIS COMMAND HANDLER
It has a parseRespCommand function takes a string command in RESP format and parses it into a vector of strings representing the individual components of the command.

Input Example :
*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n

Output Example :
["SET", "key", "value"]

Cases : 1. if it does not start with '*', it returns an empty vector.
2. It reads the number of elements in the array.
3. It iterates through each element, reading the bulk string length and the actual string value.
4. It stores each string value in a vector and returns it.

Working :
 1. It first checks if the command starts with the character '*', which indicates that it is an array of bulk strings in RESP format.
 2. It then reads the number of elements in the array by extracting the substring after '*'


#RedisDatabase : Data Structure

Key-Value(K)
key_value_store["name"] = "Alice";
key_value_store["city"] = "Berlin";

List (L)
list_store["fruit"] = {"orange", "strawberry", "mango"};
list_store["colors"] = {"red", "green", "blue"};

Hash (H)
hash_store["user:1000"] = {
    {"name", "Alice"},
    {"age", "30"},
    {"city", "Berlin"}
};

hash_store["user:1001"] = {
    {"name", "Bob"},
    {"age", "25"},
    {"city", "New York"}
};