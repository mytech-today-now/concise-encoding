Concise Text Encoding: Condensed Specification
==============================================

As the [CTE specification](cte-specification.md) has become rather dense, this document is provided as a simpler description of how CTE documents are structured. Many details are glossed over, so this document should not be considered a replacement for the [official specification](cte-specification.md).

Concise Text Encoding (CTE) is the text variant of Concise Encoding: a general purpose, human and machine friendly, compact representation of semi-structured hierarchical data.

The text format aims to present data in a human friendly way, while the 1:1 compatible [binary format](cbe-specification.md) aims for compactness and machine processing efficiency.

CTE is a lot like JSON with rich type support. The following types are natively supported:

| Type                                        | Example                                 |
| ------------------------------------------- | --------------------------------------- |
| Nil                                         | `@nil`                                  |
| Boolean                                     | `@true`                                 |
| Integer                                     | `-1_000_000_000_000_000`                |
| Float                                       | `4.8255`                                |
| [UUID](https://tools.ietf.org/html/rfc4122) | `@123e4567-e89b-12d3-a456-426655440000` |
| Time                                        | `2019-7-15/18:04:00/E/Rome`             |
| String                                      | `"A string"`                            |
| [URI](https://tools.ietf.org/html/rfc3986)  | `u"http://example.com?q=1"`             |
| Custom Type (binary encoding)               | `b"04 f6 28 3c 40 00 00 40 40"`         |
| Custom Type (text encoding)                 | `t"cplx(2.94+3i)"`                      |
| Typed Array                                 | `|u8x f1 e2 d3 c4 b5 a6 97 88|`         |
| List                                        | `[1 2 3 4]`                             |
| Map                                         | `{one=1 two=2}`                         |
| Markup                                      | `<textview height=40; Some text>`       |
| Metadata Map                                | `(_id=12345)`                           |
| Marker/Reference                            | `&a_ref:"something"`, `$a_ref`          |
| Comment                                     | `// A comment`                          |
| Multiline Comment                           | `/* A comment */`                       |



General Layout
--------------

A CTE document begins with a version specifier, followed by whitespace, and then a top-level object.

Example: A CTE version 1 document containing the string `some_object` as the top-level object:

    c1 some_object

Example: A CTE version 1 document containing a map as the top-level object:

    c1
    {
        8 = "8 bit"
        16 = "16 bit"
        32 = "32 bit"
        64 = "64 bit"
    }


### Whitespace

Documents can generally contain whitespace anywhere (specific rules [here](cte-specification.md#whitespace)). It should be used to help keep the document clear and readable by humans. Only the following characters are allowed as whitespace:

| Code Point | Name            |
| ---------- | --------------- |
| u+0009     | horizontal tab  |
| u+000a     | line feed       |
| u+000d     | carriage return |
| u+0020     | space           |



Basic Types
-----------

### Nil

Meaning "no data": `@nil`


### Boolean

Boolean types can be `@true` or `@false`


### UUID

UUID types follow the standard UUID format, except they are prefixed with `@`:

    @123e4567-e89b-12d3-a456-426655440000


### Integer

Integers are positive or negative, and can be written in different bases:

| Base | Name        | Digits           | Prefix | Example      | Decimal Equivalent |
| ---- | ----------- | ---------------- | ------ | ------------ | ------------------ |
|   2  | Binary      | 01               | `0b`   | `-0b1100`    | -12                |
|   8  | Octal       | 01234567         | `0o`   | `0o755`      | 493                |
|  10  | Decimal     | 0123456789       |        | `900000`     | 900000             |
|  16  | Hexadecimal | 0123456789abcdef | `0x`   | `0xdeadbeef` | 3735928559         |

You can use `_` as "numeric whitespace": `1_000_000`


### Floating Point

Floating point numbers are written with a whole part, a fractional part, and possibly an exponential part. Base-10 floating point marks the exponential portion using `e`, and base-16 numbers use `p`. Base-16 floating point numbers also begin with the prefix `0x`:

| Exponent | Example         |
| -------- | --------------- |
| None     | `-105.67`       |
| Base-10  | `1.944e-28`     |
| Base-16  | `-0xa.3fb8p+42` |

Floating point values can also use numeric whitespace: `-1.541_998e20`

Special floating point values:

 * `@inf`: Infinity
 * `-@inf`: Negative Infinity
 * `@nan`: Not a Number (quiet)
 * `@snan`: Not a Number (signaling)


### Date

A date contains a year, month, and day field, in that order: `2018-11-4`


### Time

A time contains an hour, minute, second, optional subsecond, and optional time zone, using a 24h clock. Subsecond values are supported down to the nanosecond.

    18:04:11
    11:59:30/America/New_York
    14:41:13.148552211/Europe/Berlin

Note: If the time zone is omitted, it is assumed to be UTC.


### Timestamp

A timestamp is a combination date and time, separated by `/`: `1985-10-26/01:20:01.105/America/Los_Angeles`


#### Time Zones

Time zones can be written as area/location, or as latitude/longitude:

| Type               | Example        |
| ------------------ | -------------- |
| Area/Location      | `Asia/Tokyo`   |
| Latitude/Longitude | `35.68/139.77` |

Time zone areas can be [abbreviated](cte-specification.md#abbreviated-areas).

Special time zones:

| Zone    | Abbreviation | Meaning                          |
| ------- | ------------ | -------------------------------- |
| `Zero`  | `Z`          | UTC                              |
| `Local` | `L`          | In the time zone of the observer |


### String

#### Unquoted String

An unquoted string begins with a non-punctuation, non-numeric letter or `_`, after which alphanumerics and the symbols `_`, `-`, `.`, and `:` are allowed:

    twenty-five
    Std:value.next
    _underscore+other.symbols
    飲み物/お茶

#### Quoted String

A quoted string begins with `"`, and continues until the next unescaped `"` character.

    "This is a string"

The `\` character acts as an escape character, with the following possible sequences:

| Sequence            | Interpretation                  |
| ------------------- | ------------------------------- |
| `\`, u+000a         | continuation                    |
| `\`, u+000d, u+000a | continuation                    |
| `\\`                | backslash (u+005c)              |
| `\"`                | double quote (u+0022)           |
| `\r`                | carriage return (u+000d)        |
| `\n`                | linefeed (u+000a)               |
| `\t`                | horizontal tab (u+0009)         |
| `\0` - `\5fffff`    | [unicode character](cte-specification.md#unicode-escape-sequences) |

#### Verbatim String

A verbatim string begins with a backtick `` ` ``. The next sequence of characters defines the end-of-string identifier, and is terminated by a whitespace character (space, tab, linefeed, or CR/LF). From that point, the string continues until that same end-of-string identifier is repeated (much like a "here" document in Bash).

Example:

```
`ZZZ
A verbatim sequence is like a "here" document in Bash.
Everything (including newlines) is interpreted literally.
In this example, verbatim processing continues until three "Z" characters are
encountered.ZZZ
```


### URI

A URI begins with `u`, and is enclosed in double-quotes: `u"http://example.com"`


### Custom

The custom types are for encoding custom data types that are not natively supported by Concise Encoding.

The custom **binary** type begins with `b`, and is enclosed in double-quotes. The contents are encoded as hex bytes to represent the array's octets:

    b"04 f6 28 3c 40 00 00 40 40"
    = example "cplx" struct{ type uint8(4), real float32(2.94), imag float32(3.0) }

The custom **text** type begins with `t`, and is enclosed in double-quotes. All double-quote `"` and backslash `\` characters must be escaped:

    t"cplx(2.94+3i)"

The general idea is to use the binary custom type for encoding into CBE, and the text custom type for encoding into CTE (to preserve human readability). Custom binary and text types must be 1:1 convertible to each other.


### Typed Array

A typed array encodes an array of values of a fixed type and size. Typed arrays are delimited by pipe (`|`) characters, containing a whitespace separated element type, followed by the whitespace separated element values:

    |type value value value ...|

The following array element types are allowed:

| Type   | Description             |
| ------ | ----------------------- |
| `b`    | Boolean                 |
| `u8`   | 8-bit unsigned integer  |
| `u16`  | 16-bit unsigned integer |
| `u32`  | 32-bit unsigned integer |
| `u64`  | 64-bit unsigned integer |
| `i8`   | 8-bit signed integer    |
| `i16`  | 16-bit signed integer   |
| `i32`  | 32-bit signed integer   |
| `i64`  | 64-bit signed integer   |
| `f16`  | 16-bit floating point   |
| `f32`  | 32-bit floating point   |
| `f64`  | 64-bit floating point   |
| `uu`   | 128-bit UUID            |

Values can be any of the representations allowed for the specified type. The following additional representations are also allowed within an array:

* UUID values within an array may optionally be represented without the initial `@` sentinel.
* Boolean values within an array may optionally be represented using `0` for false and `1` for true (in which case whitespace is optional).

Optionally, a suffix can be appended to the type specifier to indicate that all values must be considered to have an implicit prefix (only if the type supports it).

| Suffix | Implied element prefix | Example                       |
| ------ | ---------------------- | ----------------------------- |
| `b`    | `0b`                   | `|u8b 10011010 00010101|`     |
| `o`    | `0o`                   | `|i16o -7445 644|`            |
| `x`    | `0x`                   | `|f32x a.c9fp20 -1.ffe9p-40|` |

#### Examples

    |u8x 9f 47 cb 9a 3c|
    |f32 1.5 0x4.f391p100 30 9.31e-30|
    |i16 0b1001010 0o744 1000 0xffff|
    |uu 3a04f62f-cea5-4d2a-8598-bc156b99ea3b 1d4e205c-5ea3-46ea-92a3-98d9d3e6332f|
    // Whitespace wherever you like:
    |b   true  true   false  true false |
    // The same boolean array using 0 and 1:
    |b 1 1 0 1 0|
    // The same boolean array without whitespace:
    |b 11010|
    // Empty array of UUIDs:
    |uu|



Container Types
---------------

### List

A list is enclosed in `[]`, and contains whitespace separated objects:

    [one 2 3.0]


### Map

A map is enclosed in `{}`, and contains whitespace separated key-value pairs. A key-value pair's key and value components are separated by the `=` character with optional whitespace. A map key must not be a container type, the `@nil` type, or a value that evaluates to NaN (not-a-number).

    {
        name = "John Smith"
        email = u"mailto:john_smith@example.com"
        age = 35
    }

#### Keyable types

Only certain types can be used as keys in map-like containers:

* Numeric types except for NaN (not-a-number)
* Temporal types (times, dates)
* Strings
* URI
* Custom types (provided they represent keyable data)

@nil must not be used as a key, and [references](#reference) are not allowed as keys.


### Markup

A markup container stores XML-style data, which is essentially a name, an optional map of attributes, and an optional list of contents.

Markup containers are best suited for presentation data. For regular data, maps and lists are better.

The CTE encoding of a markup container is similar to XML, except:

 * There are no end tags. All data is contained within the begin `<`, content begin `;`, and end `>` characters.
 * Comments are encoded using `/*` and `*/` instead of `<!--` and `-->`, and can be nested.
 * [Unquoted strings](#unquoted-string) are allowed in markup names and attribute values.
 * Non-string types can be used in attribute names and values, under the same rules as [map](#map) keys and values.

#### Markup Structure

| Section    | Delimiter  | Type                      | Required |
| ---------- | ---------- | ------------------------- | -------- |
| Tag name   | `<`        | [Keyable](#keyable-types) | Y        |
| Attributes | whitespace | [Map](#map)               |          |
| Contents   | `;`        | [List](#list)             |          |
| End        | `>`        |                           | Y        |

Attributes and contents are optional. There must be whitespace between the container name and the attributes section (if present).

Illustration of markup encodings:

| Attributes | Children | Example                                                    |
| ---------- | -------- | ---------------------------------------------------------- |
|     N      |    N     | `<br>`                                                     |
|     Y      |    N     | `<div id=fillme>`                                          |
|     N      |    Y     | `<span;Some text here>`                                    |
|     Y      |    Y     | `<ul id=mylist style=boring; <li;first> <li;second> >`     |

#### Markup Attributes

The markup attributes section behaves exactly like a [map](#map).

#### Markup Contents

The markup contents section behaves like a list, but can contain only strings, comments, and other markup containers. Any sequences that are not a comment or markup container are considered strings.

The following escape sequences are allowed in strings:

| Sequence            | Interpretation                                                     |
| ------------------- | ------------------------------------------------------------------ |
| `\*`                | asterisk (u+002a)                                                  |
| `\/`                | slash (u+002f)                                                     |
| `\<`                | less-than (u+003c)                                                 |
| `\>`                | greater-than (u+003e)                                              |
| `\\`                | backslash (u+005c)                                                 |
| `` \` ``            | backtick (u+0060)                                                  |
| `\0` - `\5fffff`    | [unicode character](cte-specification.md#unicode-escape-sequences) |
| `` ` ``             | Begin a [verbatim string](#verbatim-string)                        |

#### Markup Example

```
c1
(xml-doctype=[html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" u"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"])
<html xmlns=u"http://www.w3.org/1999/xhtml" xml:lang=en;
    <body;
    <div id=parent style=normal ref-id=1;
      <script; `##
        document.getElementById('parent').insertAdjacentHTML('beforeend', '<div id="idChild"> content </div>');
      ##>
        Here is some text <span style=highlighted; with highlighted text> and more text.
        <br>
        <ul;
            <li id=item_a ; Item A>
            <li id=item_b ; Item B>
            <li id=item_c ; Item C>
        >

        /* MathML: ax^2 + bx + c */
        <mrow;
          <mi;a> <mo;&InvisibleTimes;> <msup; <mi;x> <mn;2> >
          <mo;+> <mi;b> <mo;&InvisibleTimes;> <mi;x>
          <mo;+> <mi;c>
        >
    >
  >
>
```



Pseudo-Objects
--------------

Pseudo-objects add additional metadata to another object, or to the document, or affect the interpreted structure of the document in some way. Pseudo-objects can be placed anywhere a real object can be placed, but do not themselves constitute objects. For example, `(begin-map) ("a key") (pseudo-object) (end-container)` is not valid, because the pseudo-object isn't a real object, and therefore doesn't count as an actual map value for key "a key".

#### Referring Pseudo-objects

_Referring_ pseudo-objects refer to the next object following at the current container level. This will either be a real object, or a visible pseudo-object

#### Invisible Pseudo-objects

_Invsible_ pseudo-objects are effectively invisible to referring pseudo-objects, and are skipped over when searching for the object that is being referred to.


### Marker

A marker is a _referring_, _invisible_ pseudo-object that tags the next object with a [marker ID](#marker-id), such that it can be referenced from another part of the document (or from a different document).

A marker begins with the marker initiator (`&`), followed immediately by a [marker ID](#marker-id), followed by the object being marked.

Example:

    [
        &big_string:"Pretend that this is a huge string"
        &1:{a = 1}
    ]

#### Marker ID

A marker ID is a unique (to the document) identifier for marked objects. A marker ID can either be a positive integer (up to 18446744073709551615, 64 bits), or a string of case-insensitive basic alphanumerics plus underscore (`[0-9A-Za-z_]`) with a minimum length of 1 and a maximum length of 30. Integer marker IDs will generally use less space in the binary format than multibyte strings.

**Note:** Marker ID comparisons are always case-insensitive.


### Reference

A reference is a _non-referring_, _visible_ pseudo-object that acts as a stand-in for an object that has been [marked](#marker) elsewhere in this or another document. This can be useful for repeating or cyclic data. Unlike other pseudo-objects, references can be used just like regular objects (for example, `(begin-map) ("a key") (reference) (end-container)` is valid). Note that references must not be used as map keys!

A reference begins with the reference initiator (`$`), followed immediately by either a [marker ID](#marker-id) or a [URI](#uri).

Example:

    {
        some_object = {
            my_string = &big_string:"Pretend that this is a huge string"
            my_map = &1:{
                a = 1
            }
        }

        reference_to_string = $big_string
        reference_to_map = $1
        reference_to_local_doc = $u"common.cte"
        reference_to_remote_doc = $u"https://somewhere.com/my_document.cbe?format=long"
        reference_to_local_doc_marker = $u"common.cte#legalese"
        reference_to_remote_doc_marker = $u"https://somewhere.com/my_document.cbe?format=long#examples"
    }


### Metadata Map

A metadata map is a _referring_, _visible_ pseudo-object containing keyed values which are to be associated with the object following the metadata map.

```
c1
// Metadata for the entire document
(
    _ct = 2017.01.14-15:22:41/Z
    _mt = 2019.08.17-12:44:31/Z
    _at = 2019.09.14-09:55:00/Z
)
{
    records = [
        // Metadata for "ABC Corp" record
        (
            _ct = 2019.05.14-10:22:55/Z
            _t = [longtime_client big_purchases]
        )
        {
            client = "ABC Corp"
            amount = 10499.28
            due = 2020.05.14
        }
    ]
}
```


### Comment

A comment is a _non-referring_, _invisible_, list-style pseudo-object that can only contain strings or other comment containers (to support nested comments).

Comments are written C-style:

* Single line: `// A comment`
* Multiline: `/* A comment */`

Comments can be nested: `/* /* */ */`



Combined Example
----------------

```
c1
// _ct is the creation time, in this case referring to the entire document
(_ct = 2019-9-1/22:14:01)
{
    /* Comments look very C-like, except:
       /* Nested comments are allowed! */
    */
    // Notice that there are no commas in maps and lists
    (metadata_about_a_list = "something interesting about a_list")
    a_list          = [1 2 "a string"]
    map             = {2=two 3=3000 1=one}
    string          = "A string value"
    boolean         = @true
    "binary int"    = -0b10001011
    "octal int"     = 0o644
    "regular int"   = -10000000
    "hex int"       = 0xfffe0001
    "decimal float" = -14.125
    "hex float"     = 0x5.1ec4p20
    uuid            = @f1ce4567-e89b-12d3-a456-426655440000
    date            = 2019-7-1
    time            = 18:04:00.940231541/E/Prague
    timestamp       = 2010-7-15/13:28:15.415942344/Z
    nil             = @nil
    bytes           = |u8x 10 ff 38 9a dd 00 4f 4f 91|
    "uint16 array"  = |u16x ff91 84c4 009f 3aa1|
    url             = u"https://example.com/"
    email           = u"mailto:me@somewhere.com"
    1.5             = "Keys don't have to be strings"
    long-string     = `ZZZ
A backtick induces verbatim processing, which in this case will continue
until three Z characters are encountered, similar to how here documents in
bash work.
You can put anything in here, including double-quote ("), or even more
backticks (`). Verbatim processing stops at the end sequence, which in this
case is three Z characters, specified earlier as a sentinel.ZZZ
    marked_object   = &id1:{
                               description = "This map will be referenced later using $id1"
                               value = -@inf
                               child_elements = @nil
                               recursive = $id1
                           }
    ref1            = $id1
    ref2            = $id1
    outside_ref     = $u"https://somewhere.else.com/path/to/document.cte#some_id"
    // The markup type is good for presentation data
    main-view       = <View;
                          <Image src=u"images/avatar-image.jpg">
                          <Text;
                              Hello! Please choose a name!
                          >
                          <TextInput id=name style={height=40 borderColor=gray}; Name me! >
                      >
}
```



License
-------

Copyright (c) 2018 Karl Stenerud. All rights reserved.

Distributed under the Creative Commons Attribution License: https://creativecommons.org/licenses/by/4.0/legalcode
License deed: https://creativecommons.org/licenses/by/4.0/