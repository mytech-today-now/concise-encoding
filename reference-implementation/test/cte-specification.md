Concise Text Encoding
=====================

Work in progress...

Number:
 * 123
 * -123
 * 123.45
 * 123.45d
 * 8f91d778x
 * 755o
 * 10001011b

Empty: empty

List: [a, b, c]

Map: {a: 1, b: 2}

Date: 1985-10-26T08:22:16.900142

Boolean:
 * t
 * f

String: "in quotes"
 * \ + \r or \n or \r\n = ignore
 * \ + any other char = literal

Array: type(a, b, c)
 * i8, i16, i32, i64, i128
 * f32, f64, f128
 * d64, d128
 * b
 * ts, tm, tu