7.5.2. FETCH Response
Contents:
message data
The FETCH response returns data about a message to the client. The data are pairs of data item names, and their values are in parentheses. This response occurs as the result of a FETCH or STORE command, as well as by a unilateral server decision (e.g., flag updates).

The current data items are:

BINARY[<section-binary>]<<number>>
An <nstring> or <literal8> expressing the content of the specified section after removing any encoding specified in the corresponding Content-Transfer-Encoding header field. If <number> is present, it refers to the offset within the DECODED section data.

If the domain of the decoded data is "8bit" and the data does not contain the NUL octet, the server SHOULD return the data in a <string> instead of a <literal8>; this allows the client to determine if the "8bit" data contains the NUL octet without having to explicitly scan the data stream for NULs.

Messaging clients and servers have been notoriously lax in their adherence to the Internet CRLF convention for terminating lines of textual data (text/* media types) in Internet protocols. When sending data in a BINARY[...] FETCH data item, servers MUST ensure that textual line-oriented sections are always transmitted using the IMAP CRLF line termination syntax, regardless of the underlying storage representation of the data on the server.

If the server does not know how to decode the section's Content-Transfer-Encoding, it MUST fail the request and issue a "NO" response that contains the "UNKNOWN-CTE" response code.

BINARY.SIZE[<section-binary>]
The size of the section after removing any encoding specified in the corresponding Content-Transfer-Encoding header field. The value returned MUST match the size of the <nstring> or <literal8> that will be returned by the corresponding FETCH BINARY request.

If the server does not know how to decode the section's Content-Transfer-Encoding, it MUST fail the request and issue a "NO" response that contains the "UNKNOWN-CTE" response code.

BODY
A form of BODYSTRUCTURE without extension data.
BODY[<section>]<<origin octet>>
A string expressing the body contents of the specified section. The string SHOULD be interpreted by the client according to the content transfer encoding, body type, and subtype.

If the origin octet is specified, this string is a substring of the entire body contents, starting at that origin octet. This means that BODY[]<0> MAY be truncated, but BODY[] is NEVER truncated.

Note: The origin octet facility MUST NOT be used by a server in a FETCH response unless the client specifically requested it by means of a FETCH of a BODY[<section>]<<partial>> data item.

8-bit textual data is permitted if a [CHARSET] identifier is part of the body parameter parenthesized list for this section. Note that headers (part specifiers HEADER or MIME, or the header portion of a MESSAGE/RFC822 or MESSAGE/GLOBAL part) MAY be in UTF-8. Note also that the [RFC5322] delimiting blank line between the header and the body is not affected by header-line subsetting; the blank line is always included as part of the header data, except in the case of a message that has no body and no blank line.

Non-textual data such as binary data MUST be transfer encoded into a textual form, such as base64, prior to being sent to the client. To derive the original binary data, the client MUST decode the transfer-encoded string.

BODYSTRUCTURE
A parenthesized list that describes the [MIME-IMB] body structure of a message. This is computed by the server by parsing the [MIME-IMB] header fields, defaulting various fields as necessary.

For example, a simple text message of 48 lines and 2279 octets can have a body structure of:

   ("TEXT" "PLAIN" ("CHARSET" "US-ASCII") NIL NIL "7BIT" 2279 48)
Multiple parts are indicated by parenthesis nesting. Instead of a body type as the first element of the parenthesized list, there is a sequence of one or more nested body structures. The second element of the parenthesized list is the multipart subtype (mixed, digest, parallel, alternative, etc.).

For example, a two-part message consisting of a text and a base64-encoded text attachment can have a body structure of:

   (("TEXT" "PLAIN" ("CHARSET" "US-ASCII") NIL NIL "7BIT" 1152 23)
    ("TEXT" "PLAIN" ("CHARSET" "US-ASCII" "NAME" "cc.diff")
    "<960723163407.20117h@cac.washington.edu>" "Compiler diff"
    "BASE64" 4554 73) "MIXED")
Extension data follows the multipart subtype. Extension data is never returned with the BODY fetch but can be returned with a BODYSTRUCTURE fetch. Extension data, if present, MUST be in the defined order. The extension data of a multipart body part are in the following order:

body parameter parenthesized list
A parenthesized list of attribute/value pairs (e.g., ("foo" "bar" "baz" "rag") where "bar" is the value of "foo", and "rag" is the value of "baz") as defined in [MIME-IMB]. Servers SHOULD decode parameter-value continuations and parameter-value character sets as described in [RFC2231], for example, if the message contains parameters "baz*0", "baz*1", and "baz*2", the server should decode them per [RFC2231], concatenate, and return the resulting value as a parameter "baz". Similarly, if the message contains parameters "foo*0*" and "foo*1*", the server should decode them per [RFC2231], convert to UTF-8, concatenate, and return the resulting value as a parameter "foo*".
body disposition
A parenthesized list, consisting of a disposition type string, followed by a parenthesized list of disposition attribute/value pairs as defined in [DISPOSITION]. Servers SHOULD decode parameter-value continuations as described in [RFC2231].
body language
A string or parenthesized list giving the body language value as defined in [LANGUAGE-TAGS].
body location
A string giving the body content URI as defined in [LOCATION].

Any following extension data are not yet defined in this version of the protocol. Such extension data can consist of zero or more NILs, strings, numbers, or potentially nested parenthesized lists of such data. Client implementations that do a BODYSTRUCTURE fetch MUST be prepared to accept such extension data. Server implementations MUST NOT send such extension data until it has been defined by a revision of this protocol.

The basic fields of a non-multipart body part are in the following order:

body type
A string giving the content media-type name as defined in [MIME-IMB].
body subtype
A string giving the content subtype name as defined in [MIME-IMB].
body parameter parenthesized list
A parenthesized list of attribute/value pairs (e.g., ("foo" "bar" "baz" "rag") where "bar" is the value of "foo", and "rag" is the value of "baz") as defined in [MIME-IMB].
body id
A string giving the Content-ID header field value as defined in Section 7 of [MIME-IMB].
body description
A string giving the Content-Description header field value as defined in Section 8 of [MIME-IMB].
body encoding
A string giving the content transfer encoding as defined in Section 6 of [MIME-IMB].
body size
A number giving the size of the body in octets. Note that this size is the size in its transfer encoding and not the resulting size after any decoding.

A body type of type MESSAGE and subtype RFC822 contains, immediately after the basic fields, the envelope structure, body structure, and size in text lines of the encapsulated message.

A body type of type TEXT contains, immediately after the basic fields, the size of the body in text lines. Note that this size is the size in its content transfer encoding and not the resulting size after any decoding.

Extension data follows the basic fields and the type-specific fields listed above. Extension data is never returned with the BODY fetch but can be returned with a BODYSTRUCTURE fetch. Extension data, if present, MUST be in the defined order.

The extension data of a non-multipart body part are in the following order:

body MD5
A string giving the body MD5 value as defined in [MD5].
body disposition
A parenthesized list with the same content and function as the body disposition for a multipart body part.
body language
A string or parenthesized list giving the body language value as defined in [LANGUAGE-TAGS].
body location
A string giving the body content URI as defined in [LOCATION].

Any following extension data are not yet defined in this version of the protocol and would be as described above under multipart extension data.

ENVELOPE
A parenthesized list that describes the envelope structure of a message. This is computed by the server by parsing the [RFC5322] header into the component parts, defaulting various fields as necessary.

The fields of the envelope structure are in the following order: date, subject, from, sender, reply-to, to, cc, bcc, in-reply-to, and message-id. The date, subject, in-reply-to, and message-id fields are strings. The from, sender, reply-to, to, cc, and bcc fields are parenthesized lists of address structures.

An address structure is a parenthesized list that describes an electronic mail address. The fields of an address structure are in the following order: display name, [SMTP] at-domain-list (source route and obs-route ABNF production from [RFC5322]), mailbox name (local-part ABNF production from [RFC5322]), and hostname.

[RFC5322] group syntax is indicated by a special form of address structure in which the hostname field is NIL. If the mailbox name field is also NIL, this is an end-of-group marker (semicolon in RFC 822 syntax). If the mailbox name field is non-NIL, this is the start of a group marker, and the mailbox name field holds the group name phrase.

If the Date, Subject, In-Reply-To, and Message-ID header fields are absent in the [RFC5322] header, the corresponding member of the envelope is NIL; if these header fields are present but empty, the corresponding member of the envelope is the empty string.

Note: some servers may return a NIL envelope member in the "present but empty" case. Clients SHOULD treat NIL and the empty string as identical.

Note: [RFC5322] requires that all messages have a valid Date header field. Therefore, for a well-formed message, the date member in the envelope cannot be NIL or the empty string. However, it can be NIL for a malformed or draft message.

Note: [RFC5322] requires that the In-Reply-To and Message-ID header fields, if present, have non-empty content. Therefore, for a well-formed message, the in-reply-to and message-id members in the envelope cannot be the empty string. However, they can still be the empty string for a malformed message.

If the From, To, Cc, and Bcc header fields are absent in the [RFC5322] header, or are present but empty, the corresponding member of the envelope is NIL.

If the Sender or Reply-To header fields are absent in the [RFC5322] header, or are present but empty, the server sets the corresponding member of the envelope to be the same value as the from member (the client is not expected to know how to do this).

Note: [RFC5322] requires that all messages have a valid From header field. Therefore, for a well-formed message, the from, sender, and reply-to members in the envelope cannot be NIL. However, they can be NIL for a malformed or draft message.

FLAGS
A parenthesized list of flags that are set for this message.
INTERNALDATE
A string representing the internal date of the message.
RFC822.SIZE
A number expressing the size of a message, as described in Section 2.3.4.
UID
A number expressing the unique identifier of the message.
If the server chooses to send unsolicited FETCH responses, they MUST include UID FETCH item. Note that this is a new requirement when compared to [RFC3501].

Example:

  S: * 23 FETCH (FLAGS (\Seen) RFC822.SIZE 44827 UID 447)