version: "3"
services:

  invidious:
    image: quay.io/invidious/invidious:master
    # image: quay.io/invidious/invidious:latest-arm64 # ARM64/AArch64 devices
    restart: unless-stopped
    volumes:
      - ./config/trusted_session:/config/trusted_session
    ports:
      - "3000:3000"
    environment:
      # Please read the following file for a comprehensive list of all available
      # configuration options and their associated syntax:
      # https://github.com/iv-org/invidious/blob/master/config/config.example.yml
      INVIDIOUS_CONFIG: |
        db:
          dbname: invidious
          user: kemal
          password: kemal
          host: invidious-db
          port: 5432
        check_tables: true
        use_innertube_for_captions: true
        signature_server: inv_sig_helper:12999
        po_token: PO_TOKEN_VALUE
        visitor_data: VISITOR_DATA_VALUE
        # external_port:
        # domain:
        # https_only: false
        # statistics_enabled: false
        hmac_key: HMAC_KEY_VALUE
    healthcheck:
      test: wget -nv --tries=1 --spider http://127.0.0.1:3000/api/v1/trending || exit 1
      interval: 30s
      timeout: 5s
      retries: 2
    logging:
      options:
        max-size: "1G"
        max-file: "4"
    depends_on:
      - invidious-db

  inv_sig_helper:
    image: quay.io/invidious/inv-sig-helper:latest
    command: ["--tcp", "0.0.0.0:12999"]
    environment:
      - RUST_LOG=info
    restart: unless-stopped
    cap_drop:
      - ALL
    read_only: true
    security_opt:
      - no-new-privileges:true

  invidious-db:
    image: docker.io/library/postgres:14
    restart: unless-stopped
    volumes:
      - postgresdata:/var/lib/postgresql/data
      - ./config/sql:/config/sql
      - ./docker/init-invidious-db.sh:/docker-entrypoint-initdb.d/init-invidious-db.sh
    environment:
      POSTGRES_DB: invidious
      POSTGRES_USER: kemal
      POSTGRES_PASSWORD: kemal
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U $$POSTGRES_USER -d $$POSTGRES_DB"]

volumes:
  postgresdata:
