#!/bin/bash

PAGES_FILENAME="enwiki-latest-page.sql.gz"
LINKS_FILENAME="enwiki-latest-pagelinks.sql.gz"
REDIRECTS_FILENAME="enwiki-latest-redirect.sql.gz"
TARGET_FILENAME="enwiki-latest-linktarget.sql.gz"

echo "Trimming Redirect File"
# page_id, page_namespace, page_title,... )
# remove lines not starting with INSERT INTO
# split multiple inserts into single inserts with one line each ),( -> )\n(
# only keep links with namespace 0: ?,0,?,...
# now delete ,0, -> ?,?,...
# now apply escape characters to string, and only keep data until the page_is_redirect column
# ?,?,... -> ?	?
# so went from INSERT INTO page (151,0,'an_example',0.......) - > 151,"an_example",0\n
time pigz  -dc $REDIRECTS_FILENAME \
	| sed -E 's/^INSERT INTO `redirect` VALUES //' \
    | sed -E 's/^\(//; s/\);?$//' \
    | sed 's/),(/\'$'\n/g' \
	| egrep "^[0-9]+,0," \
	| sed -e $"s/,0,'/\t/" \
    | sed -e "s/','.*//g" \
	| pigz --fast > redirects.tsv.gz.tmp
mv redirects.tsv.gz.tmp redirects.tsv.gz


echo "Trimming Page File"
# page_id, page_namespace, page_title, page_is_redirect,... )
# remove lines not starting with INSERT INTO
# split multiple inserts into single inserts with one line each ),( -> )\n(
# only keep links with namespace 0: ?,0,?,?
# now delete ,0, -> ?,"?,?,...
# now apply escape characters to string, and only keep data until the page_is_redirect column
# ?,"?,?,... -> ?,"?",?
# so went from INSERT INTO page (151,0,'an_example',0.......) - > 151,"an_example",0\n
time pigz  -dc $PAGES_FILENAME \
	| sed -E 's/^INSERT INTO `page` VALUES //' \
    | sed -E 's/^\(//; s/\);?$//' \
    | sed 's/),(/\'$'\n/g' \
	| egrep "^[0-9]+,0," \
	| sed -e $"s/,0,'/\t/" \
	| sed -e $"s/',[^,]*,\([01]\).*/\t\1/" \
	| pigz --fast > pages.tsv.gz.tmp
mv pages.tsv.gz.tmp pages.tsv.gz



echo "Trimming Links File"
# remove lines not starting with INSERT INTO
# split multiple inserts into single inserts with one line each ),( -> )\n(
# only keep links with namespace 0 ..,0,..
# now delete ,0, -> ,
# so went from INSERT INTO .. (151,0,1535) - > 151,1535\n
time pigz  -dc $LINKS_FILENAME \
	| sed -E 's/^INSERT INTO `pagelinks` VALUES //' \
    | sed -E 's/^\(//; s/\);?$//' \
    | sed 's/),(/\'$'\n/g' \
    | awk -F',' '$2 == 0 {print $1 "," $3}' \
    | pigz --fast > links.csv.gz.tmp
mv links.csv.gz.tmp links.csv.gz


echo "Trimming Target File"
# remove lines not starting with INSERT INTO
# split multiple inserts into single inserts with one line each '),( -> \n
# only keep links with namespace 0 ..,0,..
# now delete ,0, -> \t
# so went from INSERT INTO .. (151,0,test) - > 151	test\n
time pigz -dc $TARGET_FILENAME \
	| sed -E 's/^INSERT INTO `linktarget` VALUES //' \
    | sed -E 's/^\(//; s/\);?$//' \
	| sed "s/'),(/\n/g" \
	| egrep "^[0-9]+,0," \
	| sed -e $"s/,0,'/\t/" \
	| sed -e $"s/',[^,]*,\([01]\).*/\t\1/" \
	| pigz --fast > linktarget.tsv.gz.tmp
mv linktarget.tsv.gz.tmp linktarget.tsv.gz