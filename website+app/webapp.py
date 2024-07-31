from flask import Flask, request, render_template
from pymongo.mongo_client import MongoClient


app = Flask(__name__)
client = MongoClient("localhost", 27017)

def into_db(data, endp):
    db = client.mwt
    if endp == "before":
        try:
            db.before.insert_one(data)
            return "Success", 200
    
        except Exception as err:
            return err
        
    elif endp == "after":
        try:
            db.after.insert_one(data)
            return "Success", 200
    
        except Exception as err:
            return err

    

@app.route("/")
def landing():
    return render_template('home2.html')


@app.route("/before", methods=['POST'])
def get_data_bf():
    data = request.get_json()
    return into_db(data, "before")

@app.route("/after", methods=['POST'])
def get_data_af():
    data = request.get_json()
    return into_db(data, "after")

if __name__ == "__main__":
    app.run()