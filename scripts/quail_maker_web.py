#!/usr/bin/env python3

import csv
import os
from datetime import datetime, timedelta, timezone
from flask import Flask, send_file
from glob import glob
from tempfile import NamedTemporaryFile


def get_csv_files(directory):
    path = os.path.join(directory, "*.csv")
    files = glob(path)
    return sorted(files)


def gather_data(directory="./"):
    files = get_csv_files(directory)

    data = []

    edt = timezone(timedelta(hours=-4))
    for f in files:
        with open(f, newline="") as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                row["timestamp"] = datetime.fromisoformat(row["timestamp"]).astimezone(tz=edt)
                row["temperature"] = float(row["temperature"])
                row["rh"] = float(row["rh"])
                data.append(row)

    return data


def row_to_html(row):
    return f"""
    <tr>
        <td>{row["timestamp"].isoformat()}</td>
        <td><center>{row["temperature"]:.1f} \xb0C</center></td>
        <td><center>{row["rh"]:.1f} %</center></td>
    </tr>
    """

def populate_downloadable_csv(output_file, data):
    fieldnames = ["Timestamp", "Tempincture", "Humidity"]
    writer = csv.DictWriter(output_file, fieldnames=fieldnames)
    writer.writeheader()

    for datum in data:
        writer.writerow({
            "Timestamp": datum["timestamp"].strftime("%Y-%m-%d %H:%M:%S"),
            "Tempincture": f"{datum['temperature']:.1f}",
            "Humidity": f"{datum['rh']:.1f}"
        })


app = Flask(__name__)


@app.route("/")
def quail_log_dump():
    data = gather_data()
    html_data = "\n".join(row_to_html(row) for row in reversed(data))
    return f"""
    <h1>Hello, Quail!</h1>
    <a href="/download">Download</a>
    <table>
        <tr>
            <td><h3>Timestamp</h3></td>
            <td><h3>Tempincture</h3></td>
            <td><h3>Humidity</h3></td>
        </tr>
        {html_data}
    </table>
    """

@app.route("/download")
def download_quail_log():
    data = gather_data()
    with NamedTemporaryFile(mode="w", newline="", prefix="quail_maker_", suffix=".csv") as output_csvfile:
        populate_downloadable_csv(output_csvfile, data)
        output_csvfile.flush()
        return send_file(output_csvfile.name, as_attachment=True)
