<?php

####################################################################################
#                                                                                  #
#                        Bildschirmtricks participate V1.0.0                       #
#                                   uploader tool                                  #
#                                                                                  #
#    Copyright (C) 2008 Philipp Fabian Benedikt Maier (aka. Dexter)                #
#                                                                                  #
#    This program is free software; you can redistribute it and/or modify          #
#    it under the terms of the GNU General Public License as published by          #
#    the Free Software Foundation; either version 2 of the License, or             #
#    (at your option) any later version.                                           #
#                                                                                  #
#    This program is distributed in the hope that it will be useful,               #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of                #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
#    GNU General Public License for more details.                                  #
#                                                                                  #
#    You should have received a copy of the GNU General Public License             #
#    along with this program; if not, write to the Free Software                   #
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA    #
#                                                                                  #
#################################################################################### 

## HEADER ########################################################################## 

//User authorisation configuration
$USER_NUMBER = 1;
$USER_NAME[0] = "haspa";
$USER_PASS[0] = "post";
$USER_RANGE_FROM[0] = 23;
$USER_RANGE_TO[0] = 42;

//Error reporting configuration, uncommit this if you wish to debug
//error_reporting(E_ALL);
//ini_set('display_errors', TRUE);

//Determine page name
$pagename = $_SERVER['PHP_SELF'];

#################################################################################### 

#################################################################################### 
?>

<html>
<head>
</head>
<body>

<p>
Welcome!<br>
The Deutsche Chaospost* is proud to offer your a great offer: 
Create your own Bildschirmtrix pages and upload them to the default Ulm.
By this way it is possible to participate actively on the Bildschirmtrix project.
<br><br>
There are only a few rules:<br>
No content that violates laws!<br>
No copyrighted content!<br>
Content must be available under Creative-Commons of a semilar licence!<br>
All content you upload will be part of the official Bildschirmtrix distribution - forever!<br>
<br>
</p>

<form action="<?php echo($pagename) ?>" method="post" enctype="multipart/form-data"> 
BTX-File:<br>
<input type="file" name="btxPage"><br>
User name:<br>
<input type="text" name="btxUser" value=""><br>
Password:<br>
<input type="password" name="btxPass"value=""><br> 
<input type="submit" value="Upload"> 
</form>


<?php

	if($btxUser != "")
	{

		//Check if username and password are valid
		$btxUserValid = false;
		for($i = 0; $i<$USER_NUMBER; $i++)
			if(($btxUser == $USER_NAME[$i])&&($btxPass == $USER_PASS[$i]))
			{
				$btxUserValid = true;
				$btxUserId = $i;
				echo("User authorisation ok!<br>");
			}

		if($btxUserValid != true)
			echo("User authorisation failure!<br>");
		else
		{


			//Ensure that the user can ONLY upload ".btx" pages		
			$btxPageName = explode (".",$_FILES['btxPage']['name']);
			$btxPage = $btxPageName[0].".btx";
			echo("Filename: ".$btxPage."<br>");
			echo("Page-ID: *".$btxPageName[0]."#<br>");


			//Check if the file is in the assigned range
			echo("Your BTX-Page-ID range is from *".$USER_RANGE_FROM[$btxUserId]."# to *".$USER_RANGE_TO[$btxUserId]."#<br>");
			if(((int)$btxPageName[0] <= $USER_RANGE_TO[$btxUserId])&&((int)$btxPageName[0] >= $USER_RANGE_FROM[$btxUserId]))
			{
				echo("Uploading to Ulm.....<br>");
				move_uploaded_file($_FILES['btxPage']['tmp_name'], "./".$btxPage);
				echo("Done!<br>");
			}
			else
				echo("Page-ID is out of your assinged range!<br>");
		}
	}

?>
<br><br>



* Deutsche Chaospost is the fictive Telecommunications provider of the Chaosrepublik Deutschland. Bildschirmtrix is a fictive onlineservice of Deutsche Chaospost<br><br>

Bildschirmtrix - Bildschirmtext lives, just for one day! <a href="http://runningserver.com/?page=runningserver.content.thelab.bildschirmtrix">Learn more!</a>



</body>
<?php
#################################################################################### 
?>