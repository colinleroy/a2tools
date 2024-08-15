#!/bin/bash

which docker-compose > /dev/null
if [ "$?" != "0" ]; then
	echo Please install docker-compose
	exit 1
fi

# Get po_token and visitor_data
docker run quay.io/invidious/youtube-trusted-session-generator > trusted-session.txt
if [ "$?" != "0" ]; then
	echo Error getting trusted session
	exit 1
fi

# Setup variables
po_token=$(grep po_token trusted-session.txt |sed 's/^po_token: //')
visitor_data=$(grep visitor_data trusted-session.txt |sed 's/^visitor_data: //')
hmac_key=$(pwgen 20 1)

# Error checking
if [ "$hmac_key" = "" ]; then
	echo Could not get hmac_key
	exit 1
fi
if [ "$po_token" = "" ]; then
	echo Could not get po_token
	exit 1
fi
if [ "$visitor_data" = "" ]; then
	echo Could not get visitor_data
	exit 1
fi

# Update docker-compose.yml
cp docker-compose.yml.tpl docker-compose.yml
sed -i "s/HMAC_KEY_VALUE/$hmac_key/" docker-compose.yml
sed -i "s/PO_TOKEN_VALUE/$po_token/" docker-compose.yml
sed -i "s/VISITOR_DATA_VALUE/$visitor_data/" docker-compose.yml

# Let's roll
docker-compose up -d
