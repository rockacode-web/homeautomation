 #!/usr/bin/python3


#################################################################################################################################################
#                                                    CLASSES CONTAINING ALL THE APP FUNCTIONS                                                                                                    #
#################################################################################################################################################


class DB:

    def __init__(self,Config):

        from math import floor
        from os import getcwd
        from os.path import join
        from json import loads, dumps, dump
        from datetime import timedelta, datetime, timezone 
        from pymongo import MongoClient , errors, ReturnDocument
        from urllib import parse
        from urllib.request import  urlopen 
        from bson.objectid import ObjectId  
       
      
        self.Config                         = Config
        self.getcwd                         = getcwd
        self.join                           = join 
        self.floor                      	= floor 
        self.loads                      	= loads
        self.dumps                      	= dumps
        self.dump                       	= dump  
        self.datetime                       = datetime
        self.ObjectId                       = ObjectId 
        self.server			                = Config.DB_SERVER
        self.port			                = Config.DB_PORT
        self.username                   	= parse.quote_plus(Config.DB_USERNAME) if Config.DB_USERNAME else ""
        self.password                   	= parse.quote_plus(Config.DB_PASSWORD) if Config.DB_PASSWORD else ""
        self.remoteMongo                	= MongoClient
        self.ReturnDocument                 = ReturnDocument
        self.PyMongoError               	= errors.PyMongoError
        self.BulkWriteError             	= errors.BulkWriteError  
        self.tls                            = False # MUST SET TO TRUE IN PRODUCTION


    def __del__(self):
            # Delete class instance to free resources
            pass
 


    ####################
    # LAB 4 FUNCTIONS  #
    ####################
    
    # 1. CREATE FUNCTION TO INSERT DATA IN TO THE RADAR COLLECTION
    def insert_radar(self, data):
        client = None
        try:
            client = self._client()
            collection = client["ELET2415"]["radar"]
            result = collection.insert_one(data)
            return bool(result.inserted_id)
        except self.PyMongoError as error:
            print(f"DB insert_radar error: {error}")
            return False
        finally:
            if client:
                client.close()

    
    # 2. CREATE FUNCTION TO RETRIEVE ALL DOCUMENTS FROM RADAR COLLECT BETWEEN SPECIFIED DATE RANGE. MUST RETURN A LIST OF DOCUMENTS
    def get_radar_between(self, start, end):
        client = None
        try:
            client = self._client()
            collection = client["ELET2415"]["radar"]
            query = {"timestamp": {"$gte": int(start), "$lte": int(end)}}
            projection = {"_id": 0}
            cursor = collection.find(query, projection).sort("timestamp", 1)
            return list(cursor)
        except self.PyMongoError as error:
            print(f"DB get_radar_between error: {error}")
            return []
        finally:
            if client:
                client.close()


    # 3. CREATE A FUNCTION TO COMPUTE THE ARITHMETIC AVERAGE ON THE 'reserve' FEILED/VARIABLE, USING ALL DOCUMENTS FOUND BETWEEN SPECIFIED START AND END TIMESTAMPS. RETURNS A LIST WITH A SINGLE OBJECT INSIDE
    def get_reserve_average_between(self, start, end):
        client = None
        try:
            client = self._client()
            collection = client["ELET2415"]["radar"]
            pipeline = [
                {"$match": {"timestamp": {"$gte": int(start), "$lte": int(end)}}},
                {"$group": {"_id": None, "average": {"$avg": "$reserve"}}},
                {"$project": {"_id": 0, "average": 1}},
            ]
            return list(collection.aggregate(pipeline))
        except self.PyMongoError as error:
            print(f"DB get_reserve_average_between error: {error}")
            return []
        finally:
            if client:
                client.close()

    
    # 4. CREATE A FUNCTION THAT INSERT/UPDATE A SINGLE DOCUMENT IN THE 'code' COLLECTION WITH THE PROVIDED PASSCODE
    def set_passcode(self, passcode):
        client = None
        try:
            client = self._client()
            collection = client["ELET2415"]["code"]
            result = collection.find_one_and_update(
                {"type": "passcode"},
                {"$set": {"code": str(passcode)}},
                projection={"_id": 0},
                upsert=True,
                return_document=self.ReturnDocument.AFTER,
            )
            return bool(result and result.get("code") == str(passcode))
        except self.PyMongoError as error:
            print(f"DB set_passcode error: {error}")
            return False
        finally:
            if client:
                client.close()
   
    
    # 5. CREATE A FUNCTION THAT RETURNS A COUNT, OF THE NUMBER OF DOCUMENTS FOUND IN THE 'code' COLLECTION WHERE THE 'code' FEILD EQUALS TO THE PROVIDED PASSCODE.
    #    REMEMBER, THE SCHEMA FOR THE SINGLE DOCUMENT IN THE 'code' COLLECTION IS {"type":"passcode","code":"0070"}
    def count_passcode(self, passcode):
        client = None
        try:
            client = self._client()
            collection = client["ELET2415"]["code"]
            return collection.count_documents({"type": "passcode", "code": str(passcode)}, limit=1)
        except self.PyMongoError as error:
            print(f"DB count_passcode error: {error}")
            return 0
        finally:
            if client:
                client.close()

    def _client(self):
        host = self.server or "localhost"
        port = self.port or 27017
        username = self.username or ""
        password = self.password or ""

        if username and password:
            uri = f"mongodb://{username}:{password}@{host}:{port}/?authSource=admin"
        else:
            uri = f"mongodb://{host}:{port}/"

        return self.remoteMongo(uri, tls=self.tls, serverSelectionTimeoutMS=5000)


   



def main():
    from config import Config
    from time import time, ctime, sleep
    from math import floor
    from datetime import datetime, timedelta
    one = DB(Config)
 
 
    start = time() 
    end = time()
    print(f"completed in: {end - start} seconds")
    
if __name__ == '__main__':
    main()


    
