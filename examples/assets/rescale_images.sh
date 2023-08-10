#!/bin/bash
#
# Rescaling all images of given folder to new size
# without interpolation.

CONVERT_CMD_FORMAT='convert "%s" -interpolate nearest-neighbor -interpolative-resize "%s" "%s"'
CONVERT_CMD_FORMAT='convert -define colorspace:auto-grayscale=false %s -interpolate nearest-neighbor -interpolative-resize %s %s'

SOURCE_FOLDER="$1"
GEOMETRY="$2"
TARGET_FOLDER="$3"


main() {

	SOURCE_IMAGES=$(cd "$SOURCE_FOLDER" && find . -name "*.png")
	for SOURCE_IMAGE in $SOURCE_IMAGES ; do
		# echo "$SOURCE_IMAGE" # Name without SOURCE_FOLDER prefix

		# Create target folder if needed
		TARGET_SUBFOLDER=$(dirname "${TARGET_FOLDER}/$SOURCE_IMAGE")
		if [ ! -d "$TARGET_SUBFOLDER" ] ; then
			mkdir -p "$TARGET_SUBFOLDER"
		fi

		CONVERT_CMD=$(printf "$CONVERT_CMD_FORMAT" "$SOURCE_FOLDER/$SOURCE_IMAGE" "$GEOMETRY" "$TARGET_FOLDER/$SOURCE_IMAGE")
		$CONVERT_CMD
		test "$?" -ne 0 && echo "Convert failed for $SOURCE_IMAGE"
	done
}

main
