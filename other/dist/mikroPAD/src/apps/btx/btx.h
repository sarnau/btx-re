/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                btx service control                               #
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
   #################################################################################### */


/* ## HEADER ########################################################################## */
#ifndef BTX_H
#define BTX_H

#define HISTORY_BLOCKED 1
#define HISTORY_UNBLOCKED 0

/* Resolve Ulm IP-Adress */
int applicationBtxResolveUlm(unsigned long *ulmIp);

/* Convert a btx page identifier (e.g *123456#) to an http-style url */
int applicationBtxGenUrl(char *btxPageId, char *url);

/* Check if the page id is valid */
int applicationBtxCheckPageId(char *btxPageId);

/* Check if the hyperlink id is valid */
int applicationBtxCheckHyperlinkId(char *btxHyperlinkId);

/* Store a btx page in history. */
int applicationBtxHistoryPush(char *btxPageId);

/* Restore a btx page from history. */
int applicationBtxHistoryPop(char *btxPageId);

/* Block/Unblock history */
int applicationBtxHistoryBlockCtrl(int mode);

#endif /*BTX_H*/
/* #################################################################################### */
