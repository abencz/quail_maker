#!/usr/bin/env python3

import argparse
import csv
import requests
import time
from datetime import datetime, timezone


# Example curl command from Particle documentation
# curl "https://api.particle.io/v1/devices/0123456789abcdef01234567/temperature?access_token=1234"


def get_variable(device_id, access_token, variable, retries=5):
    url = f"https://api.particle.io/v1/devices/{device_id}/{variable}?access_token={access_token}"
    
    while retries > 0:
        try:
            r = requests.get(url)
            r.raise_for_status()
        except requests.exceptions.HTTPError:
            print(f"Failed to fetch variable '{variable}' at {datetime.now().isoformat()} due to {r.status_code}")
        except Exception as e:
            print(f"Failed to fetch variable '{variable}' at {datetime.now().isoformat()} due to {e}")
        else:
            result = r.json()
            var_value = float(result["result"])
            utc = datetime.fromisoformat(result["coreInfo"]["last_heard"][:-1])
            local = utc.replace(tzinfo=timezone.utc).astimezone(tz=None)
            return (var_value, local)

        retries -= 1
        time.sleep(3)

    return None


def log_data(output_file, device_id, access_token, log_delay=3600):
    fieldnames = ["timestamp", "temperature", "rh"]
    writer = csv.DictWriter(output_file, fieldnames=fieldnames)
    writer.writeheader()
    last_timestamp = datetime(2000, 1, 1)

    while True:
        try:
            temp, timestamp = get_variable(device_id, access_token, "temperature")
            rh, timestamp = get_variable(device_id, access_token, "rh")
        except TypeError:
            time.sleep(log_delay)
            continue

        if last_timestamp == timestamp:
            print(f"Timestamp unchanged from last API call")
        else:
            last_timestamp = timestamp

            writer.writerow({
                "timestamp": timestamp.isoformat(),
                "temperature": temp,
                "rh": rh
            })
            output_file.flush()
            print(f"[{timestamp.isoformat()}, {temp}, {rh}]")
        time.sleep(log_delay)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("device_id", type=str)
    parser.add_argument("access_token", type=str)
    parser.add_argument("-f", "--file-prefix", type=str, default="quail_maker")
    parser.add_argument("-d", "--log-delay", type=int, default=300)

    return parser.parse_args()


def main():
    args = parse_args()

    filename = f"{args.file_prefix}-{datetime.now().timestamp()}.csv"

    with open(filename, "w", newline="") as csvfile:
        log_data(csvfile, args.device_id, args.access_token, args.log_delay)


if __name__ == "__main__":
    main()
