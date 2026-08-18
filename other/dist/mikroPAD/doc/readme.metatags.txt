 


==Abstract==
This readme file illustrates all important metaTag control functions a btxml browser should feature. All the metaTag listed below are also implemented in the mikroPAD hardware.

Note: If you read the documentation the first time - please read readme.btxml.txt first!



==Load options==
Sometimes it is necessary to construct an automated forware to another BTX site. This can be done through the load_page and load_timeout parameters as shown in the following Example:

<meta name="load_page" content="*seite2#">  <!-- Load the page with the id *seite2# ... -->
<meta name="load_timeout" content="5"> <!-- ...in about 5 seconds --->

Note: If you leave the load_page meta tag away you can use load_timeout to lock the user input
      for a given time period. 

==Hyperlinks==
The linking concept of a videotex system is far different from the concept we know it from webbrowsers. On a videotex terminal it is not possible to click on something. Videotex uses numbers that must be entered at a prompt instead. 

Here is an exemple:

   On the screen:

   Cinema-Program: 01
   TV-Program:     02
   Local-events:   03

   Please select: _

So if you enter 02 for example to see the TV-Program you will be redirected to a page where the TV-Program is. This might be *tvprog# To make this concept work the system must know which site is behind which number. To do so you have to register the sites in a metatag of the structure:

<meta name="hyperlinks" content="[number][pageId][,[number][pageId],[number][pageId],...]">
Here is an example: <meta name="hyperlinks" content="01*cinemaprog#,02*tvprog#,03*localevents#">


==Nextpage==
As you know, a vidotex system can only display a small amount of text. To make it easier to offer longer texts there is a possibility to space the text on more than one page. If you do so you want to have a function that makes it easy for the user to "turn the page" On a videotex system this function is activated by pressing the # (terminator) key. If you want to make use of this function on your videotex page. You must tell the system what the next page is by registering a nextpage in a meta tag:

<meta name="next_page" content="[pageId]">

Here is an example:

<meta name="next_page" content="*seite2#">


==Disconnect==
It is possible to force a disconnect remotely via an meta tag. It is possible delay the disconnect to give the user a chance to read an error message text.

<meta name="disconnect" content="[seconds]">

Here is an example

<meta name="disconnect" content="3">









