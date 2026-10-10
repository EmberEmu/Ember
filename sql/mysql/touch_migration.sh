#!/usr/bin/env bash

case "${1:-}" in world|login)
	nodb=false ;;
	*)

	nodb=true

	printf '%s\n' \
		"No valid database argument specified (world or login)," \
		"file will be created in the current directory instead." \
		"Please move it to the desired directory." >&2

	read -r -p "Press any key to continue..."
	;;
esac

# Get the current UTC timestamp.
UTC=$(date -u '+%Y%m%d%H%M%S') || {
	echo "Error: Could not determine the current UTC date." >&2
	exit 1
}

# Get the current Git commit and tag.
COMMIT=$(git rev-parse HEAD) || {
	echo "Error: Could not determine the Git commit." >&2
	exit 1
}

TAG=$(git describe --tags) || {
	echo "Error: Could not determine the Git tag." >&2
	exit 1
}

output="${UTC}_${TAG}_${COMMIT}.sql"

if [[ "$nodb" == false ]]; then
	file="${1}/migrations/${output}"
else
	file="${output}"
fi

if ! printf '%s\n' '-- Paste your migration query below' >> "$file"; then
	echo "Error: Could not write to '$file'." >&2
	exit 1
fi

printf 'Created migration file: %s\n' "$file"