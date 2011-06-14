#
# This is a quick script to produce an HTML page from sitecopy -ll mode.
# May be copied, redistributed, modified etc. under the terms
# of the GNU GPL, see the COPYING file for full details.
#                     Copyright (C) 1998-2002 Joe Orton
#
# To use it, run:
#
#  sitecopy --flatlist sitename | gawk -f changes.awk > changes.html
#
#  [mawk (v1.3.3) doesn't work: lacks strftime.  Admittedly strftime
#   is used only for the "generated <strftime> by sitecopy" message
#   at the end.  Can replace with something like `date | getline now',
#   which should work for gawk and mawk but not e.g. Solaris awk.
#   It appears that there is no portable way of getting current time
#   in awk.  However, see below BUGS list.
#
#   Of course, what we really want is when sitecopy was run,
#   which needn't be the same as when this script is run.
#
#
# BUGS:
#
#  - Doesn't make any provision for strange filenames (containing
#    vertical bar (`|') or HTML-breaking things like `<', `&', `"'
#    etc.).
#
#    (Could be addressed by doing HTML-encoding of filenames from within
#    sitecopy code, including encoding any `|' in filenames.)
#
#  - Note that the timestamp in the output is when this awk script is
#    run, not when `sitecopy --flatlist' was run.  (This isn't a
#    problem if you use the suggested pipe command, it's only relevant
#    if you run this script on previously-generated flatlist files.)
#
#    (Could be addressed by adding date information to
#    `sitecopy --flatlist' output.)
#

BEGIN {
    # The field separator is the vertical bar
    FS = "|";
}

# This is called to print an item.
#   hyperlink == 1 if the item should be hyperlinked,
#  else 0.
function printitem( name ) {
    printf( "<li>" );
    if( hyperlink ) printf( "<a href=\"%s/%s\">", url, name );
    printf( "%s", name );
    if( hyperlink ) printf( "</a>" );
    printf( "\n" );
}

# Called to print a MOVED item, with the new name of the 
# file and the old name
function printmoved( name, oldname ) {
    printf( "<li>%s to <a href=\"%s/%s\">%s</a>\n", oldname, url, name, name );
}

/^sitestart/ {
    print "<html>";
    print "<head>";
    print "<title>Recent Changes</title>";
    print "</head>";
    print "<body>";
    print "<h1>Recent Changes</h1>";
    url = $3
}

/^siteend/ {
    
    if( $2 == "unchanged" ) {
	print "No changes have been recently made to the site.";
    }

    print "<hr>";
    
    # Shameless plug. Remove it, go on, do it now.
    # Just comment out the next four lines with #'s like these ones.
    print "<div align=\"right\">";
    printf( "Generated %s by ", strftime() );
    print "<a href=\"http://www.lyra.org/sitecopy/\">sitecopy</a>"
    print "</div>";
    
    print "</body>";
    print "</html>";

}

/^sectstart/ {
    hyperlink = 1;
    if( $2 == "added" ) {
	print "<h2>Added Items</h2>";
    } else if( $2 == "changed" ) {
	print "<h2>Changed Items</h2>";
    } else if( $2 == "moved" ) {
	print "<h2>Moved Items</h2>";
    } else if( $2 == "deleted" ) {
	print "<h2>Deleted Items</h2>";
	hyperlink = false;
    }
    print "<ul>"
}

/^sectend/ {
    print "</ul>";
}

/^item/ {
    if( $3 != "" ) {
	printmoved( $2, $3 );
    } else {
	printitem( $2 );
    }
}

