

BTX/ML RFC
=================================================================================

==Abstract==
To attach bildschirmtext terminals to a http-server there was a need for a suitable
markup language to describe the cept/btx pages that are displayed on the terminal.
We all know how to write HTML webpages so i thought that it is a great idea to
create a bastard of html and the CEPT T/CD 6-1 specification. I call it BTX/ML
what stands for "BildschirmTeXt Markup Languate"



==Case sensitivy==
We all know that html is a non case senstive formating language. no one cares
about if you write <HTML> or <html>. That is not true for btxml. Here it is
very important that you write <cept> and not <CEPT> or <Cept>. Otherwise the page
will be rejected by the parser.



==Paramter syntax==
Btxml up to now has no tags that can take parameters in the <body> section. But
the <head> section has meta-tags and these tags take two parameters. Hoewever.
If you want to issue a parameter it is very important that you write it in "..."

For example <foo bar="23"> and not <foo bar=23>.

If you do it wrong your page will be rejected with a parse error.



==Comments==
Comments have the same structure as in html and are applied like this:

	<!-- this is a comment -->

Comments are stripped before the code is parsed so they may contain format
tags as well as control tags.



==HTTP Header==
The http-header does not need to be removed bevore handing the page over to the
parser. The parser ignors the header data if present. 



==Main structure==
The main structure is like we know it from html pages. The only difference is that
we opon the page with <cept> insted of <html>

Here is an example:

<cept>
<head>

  [[meta tags]]

</head>
<body>

  [[BTX/ML formatted page]]

</body>
</cept>

Note: The parser is much more restrictive than you know it from webbrowesers. If
      on of the structure tags is missing or wrong the terminal will not display
      the page.

Note: The </cept> tag is also used (If all other end-of-file detections fail)
      to tell the terminal that the http transmission is over. If it is missing
      there is an additional risk that the terminal will hang.


==Format tags and text==
Formt tags are applied mostly like you know it from html. Every tag stands exactly
for one format character that is sent to the terminal. The format characters are
named exactly like they are described in T/TE 06-01 / T/CD 06-01 / ETS 300 072. you 
just can look up a specific format caracter in yor these documents and apply it
like an html tag. 

Note: Whitespaces are handled as non printable characters, so you MUST use the
      <sp> tag to produce a whits space like this: GNU<sp>Not<sp>Unix.

Note: The fullscreen color format characters unfortunately labeled exactly like 
      the normal color characters. To seperate them correctly just add an "f" at
      the beginning.

      An exemple:
      blb = BLue Background (normal)
      fblb = BLue Background (fullscreen)

A simple example: NormalSize<dbs>DoubleSize<nsz>NormalSize
You also can apply format characters by thier hex values: <0x23><0x42>

Note: If you apply hex codes you must write them in small letters with leading
      zeros.

      Wrong:   <0X2a>, <0x2B>, <0xF>
      Correct: <0x2a>, <0x2b>, <0x0F>



==Head data structure==
The head contains the meta data of a page. The syntax is nearly the same as we
know from html:

<meta name="[name of the identifier]" content="[data string]">

But there are a few restrictions. name and content may not exeed a limit of 256 
characters and there are not more than 8 meta tags  possible. If one of that
restriction is exeeded the parser will reject the page.



02082015 Philipp Fabian Benedikt Maier


