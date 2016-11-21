#            Ericsson AB
GIT=`which git`
usage="Usage: submit-review.sh [-t] [-r rev] [-d dest]"
while getopts ":tcor:d:" opt; do
if [ $cs -eq 0 ]; then
patchbomb_check=$("$GIT" send-email --help > /dev/null 2>&1)
if [ $? -ne 0 ]; then
        echo "The git send-email command isn't installed on your system"
        echo "Please run: sudo apt-get install git-email"
        exit 1
if [ $cs -eq 1 ]; then
	if [ -z "$rev" ]; then
		rev="HEAD"
	if echo "$rev" | grep -q '\.'; then
	    true
	else
	    if echo "$rev" | grep -q ":"; then
		cs1=`echo $rev | awk -F ":" '{ print $1 }'`
		cs2=`echo $rev | awk -F ":" '{ print $2 }'`
		rev="${cs1}^..${cs2}"
	    else
		rev="${rev}^..${rev}"
	    fi
	fi
	echo "Exporting changeset(s) '$rev' for review"
		mkdir -p "$rr"
if [ "$cs" -eq 1 ]; then
	"$GIT" format-patch --numbered --cover-letter -o "$rr" "$rev"
fi
summary=$("$GIT" log --pretty=format:'%s' "$rev" | head -1)
if [ -z "$summary" ]; then
    summary="*** FILL ME ***"
fi
ticket=$("$GIT" log "$rev" --pretty='format:%s' | grep -E '\[#[0-9]+\]' | head -1|sed -e 's/.*\[#//' | sed -e 's/\].*//')
if [ -z "$ticket" ]; then
    ticket="*** IF ANY LIST THE # ***"
cat <<ETX >> $rr/rr.patch
Summary: $summary
Review request for Ticket(s): $ticket
Peer Reviewer(s): *** LIST THE TECH REVIEWER(S) / MAINTAINER(S) HERE ***
Pull request to: *** LIST THE PERSON WITH PUSH ACCESS HERE ***
Affected branch(es): *** LIST ALL AFFECTED BRANCH(ES) ***
Development branch: *** IF ANY GIVE THE REPO URL ***
*** EXPLAIN/COMMENT THE PATCH SERIES HERE ***
"$GIT" log --pretty=format:'changeset %H%nAuthor:%x09%cn <%ce>%nDate:%x09%cD%n%n%B%n%n' "$rev" >> "$rr"/rr.patch
cat <<ETX >> $rr/rr.patch
new=`egrep -A 3 -s '^new file mode ' $rr/*.patch | grep -s '\+++ b/' | awk -F "b/" '{ print $2 }' | sort -u`
	echo "" >> $rr/rr.patch
	echo "Added Files:" >> $rr/rr.patch
	echo "------------" >> $rr/rr.patch
	done >> $rr/rr.patch
	echo "" >> $rr/rr.patch
cat <<ETX >> $rr/rr.patch
del=`egrep -A 2 -s '^deleted file mode ' $rr/*.patch | grep -s '\--- a/' | awk -F "a/" '{ print $2 }' | sort -u`
	echo "" >> $rr/rr.patch
	echo "Removed Files:" >> $rr/rr.patch
	echo "--------------" >> $rr/rr.patch
	done >> $rr/rr.patch
	echo "" >> $rr/rr.patch
cat <<ETX >> $rr/rr.patch
if [ $cs -eq 1 ]; then
    "$GIT" diff --stat "$rev" >> $rr/rr.patch
cat <<ETX >> $rr/rr.patch
*** LIST THE COMMAND LINE TOOLS/STEPS TO TEST YOUR CHANGES ***
*** PASTE COMMAND OUTPUTS / TEST RESULTS ***
*** HOW MANY DAYS BEFORE PUSHING, CONSENSUS ETC ***
___ You have a misconfigured ~/.gitconfig file (i.e. user.name, user.email etc)
"$EDITOR" "$rr"/rr.patch
        read -p "Subject: Review Request for " -e subject
        read -p "To: " -e toline
if [ "$cs" -eq 1 ]; then
	"$GIT" format-patch --numbered --cover-letter --subject="PATCH" --to "$toline" --cc "opensaf-devel@lists.sourceforge.net" -o "$rr" "$rev"
sed -i -e "s/\*\*\* SUBJECT HERE \*\*\*/Review Request for $subject/" "$rr"/0000-cover-letter.patch
sed -i -e '/^\*\*\* BLURB HERE \*\*\*$/,$d' "$rr"/0000-cover-letter.patch
cat "$rr"/rr.patch >> "$rr"/0000-cover-letter.patch
rm -f "$rr"/rr.patch*
	echo "Email thread dumped into $rr/"
	$GIT send-email --dry-run --no-format-patch --to "$toline" --cc "opensaf-devel@lists.sourceforge.net" "$rr"
	$GIT send-email --no-format-patch --confirm=never --to "$toline" --cc "opensaf-devel@lists.sourceforge.net" "$rr"
	rm -f "$rr"/*.patch
	rmdir "$rr"