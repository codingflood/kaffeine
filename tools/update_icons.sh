#!/bin/sh
set -eu

#ICONSET="Oxygen"
ICONSET="Breeze"
ORIGIN="local"

# Those two icons are not found on Oxygem. They're modified versions
# of the icons found at Oxygen:
#	actions-audio-radio-encrypted
#	actions-video-television-encrypted
#

ICONS="apps-kaffeine mimetypes-application-x-subrip actions-arrow-left actions-arrow-right devices-audio-card status-audio-volume-high status-audio-volume-low status-audio-volume-medium status-audio-volume-muted actions-configure actions-dialog-cancel status-dialog-error status-dialog-information actions-dialog-ok-apply actions-document-open-folder actions-document-save actions-edit-clear-list actions-edit-delete actions-edit-find actions-edit-rename actions-edit-undo actions-format-justify-center actions-go-jump actions-list-add devices-media-optical devices-media-optical-audio devices-media-optical-video actions-media-playback-pause actions-media-playback-start actions-media-playback-stop actions-media-record actions-media-skip-backward actions-media-skip-forward actions-page-zoom actions-player-time places-start-here-kde mimetypes-text-html actions-text-speak devices-video-television mimetypes-video-x-generic actions-view-fullscreen actions-view-list-details actions-view-media-playlist actions-view-pim-calendar actions-view-refresh actions-view-restore"

if [ "$ICONSET" == "Oxygen" ]; then
	ICONS="$ICONS status-media-playlist-repeat status-media-playlist-shuffle"

	# To use the latest icons downloaded directly from the repository
	if [ "$ORIGIN" == "local" ]; then
		ICONS_URL="file://$(pwd)/../oxygen-icons5/"
	else
		ICONS_URL="https://quickgit.kde.org/?p=oxygen-icons5.git&a=blob&f="
	fi
else
	if [ "$ORIGIN" == "local" ]; then
		ICONS_URL="file://$(pwd)/../breeze-icons/icons/"
	else
		ICONS="$ICONS actions-media-playlist-repeat actions-media-playlist-shuffle"
	fi
fi

if [ "$ORIGIN" == "local" ]; then
	ICONS_URL_END=""
else
	ICONS_URL_END="&o=plain"
fi

if [ "$(grep KAFFEINE_MAJOR_VERSION CMakeLists.txt)" == "" ]; then
  echo "Entering into the Kaffeine dir"
  cd kaffeine
fi

rm -f $(for i in $ICONS; do echo icons/*-$i.*; done)

if [ `find /usr/share/icons/oxygen | grep -i kaffeine | wc --lines` != 6 ] ; then
	echo "recheck number of icons"
	exit 1
fi

FILES=""
ICONS_NOT_FOUND=""
for i in $ICONS; do
	FOUND=""
	j=$(echo $i|sed "s,-,/,")
	curl -s "${ICONS_URL}scalable/$j.svgz${ICONS_URL_END}" -o icons/sc-$i.svgz && FOUND=1 && FILES="$FILES sc-$i.svgz" || true

	if [ "$FOUND" == "1" ]; then echo "Found $i.svgz"; continue; fi

	curl -s "${ICONS_URL}scalable/$j.svg${ICONS_URL_END}" -o icons/sc-$i.svg && FOUND=1 && FILES="$FILES sc-$i.svg" || true

	if [ "$FOUND" == "1" ]; then echo "Found $i.svg"; continue; fi

	for SIZE in 16 22 32 48 64 128 ; do
		j=$(echo $i|sed "s,-,/,")
		# Oxygen path
		curl -s "${ICONS_URL}${SIZE}x$SIZE/$j.png${ICONS_URL_END}" -o icons/$SIZE-$i.png && FOUND=1 && FILES="$FILES $SIZE-$i.png" || true
		if [ "$FOUND" == "" ]; then
			# Breeze path
			j=$(echo $j|sed "s,/,/${SIZE}/,")
			curl -s "${ICONS_URL}$j.svg${ICONS_URL_END}" -o icons/sc-$i.svg && FOUND=1 && FILES="$FILES sc-$i.svg" || true
		fi
	done

	if [ "$FOUND" == "1" ]; then echo "Found $i.png on several sizes"; continue; fi

	ICONS_NOT_FOUND="$ICONS_NOT_FOUND $i"
done

cat > icons/CMakeLists.txt << EOF
# Auto-generated by ../tools/update_icons.sh
#
# Breeze Icons are developed by The KDE Visual Design Group.
# All icons are licensed under LGPL3+

ecm_install_icons(ICONS
	# This icon is a merge of Breeze audio-mp4.svg with lock.svg
	sc-actions-audio-radio-encrypted.svg
	# This icon is a merge of Breeze sc-devices-video-television.svg with lock.svg
	sc-actions-video-television-encrypted.svg

	# Those icons are copied as-is from ${ICONSET} theme

EOF

# Remve duplicated files
FILES=$(echo "$FILES" | tr ' ' '\n' | sort -u | tr '\n' ' ')

for i in $FILES; do
	echo -e "\t$i" >> icons/CMakeLists.txt
done

echo -e "\tDESTINATION \${ICON_INSTALL_DIR}\n)" >> icons/CMakeLists.txt

# Add the two extra files
FILES="$FILES sc-actions-audio-radio-encrypted.svg sc-actions-video-television-encrypted.svg"

echo "<!DOCTYPE RCC><RCC version=\"1.0\">" >src/kaffeine.qrc
for i in $FILES; do
	alias=$(echo $i|cut -d'-' -f 3-|cut -d'.' -f1)
	echo -e "\t<qresource>\n\t\t<file alias=\"$alias\">../icons/$i</file>\n\t</qresource>"
done >>src/kaffeine.qrc
echo "</RCC>" >>src/kaffeine.qrc

if [ "$ICONS_NOT_FOUND" != "" ]; then
	echo "WARNING: Icons not found: $ICONS_NOT_FOUND"
fi
