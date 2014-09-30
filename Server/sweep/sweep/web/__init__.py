from flask import Flask, redirect
from sweep.tasks import l2

app = Flask(__name__)

@app.route("/")
def hello():
    mirror_base = "http://mesonet-nexrad.agron.iastate.edu/level2/raw/"
    sites = l2.RadarSites(mirror_base)
    results = sites.update(set(['KMPX']))
    KMPX = results['KMPX']
    return redirect(KMPX.get())

if __name__ == "__main__":
    app.run()
