# Import modules
from tweepy.streaming import StreamListener
from tweepy import OAuthHandler
from tweepy import Stream
from threading import Lock
from threading import Thread
import time
import serial
import paralleldots

EXECUTION_TIME = 25 #seconds
PORT = 'COM12'
BAUD_RATE = 9600
CHUNK_SIZE = 63
TERM_CHAR = "\r"
BOT_HANDLE = "@BotCornell"
EMPTY_CHUNK = "--"

## Twitter API Keys
consumer_key = 'your consumer key'
consumer_secret = 'your consumer secret'
access_token = 'your access token'
access_token_secret = 'your access token secret'

## Parallel
paralleldots_key = 'your paralleldots key'


class StreamListener(StreamListener):
    """ A listener handles tweets that are received from the stream.
    """

    def __init__(self, batchedtweets, lock):
    	super().__init__()
    	self.batchedtweets = batchedtweets
    	self.lock = lock

    def on_status(self, status):
    	try:
    		tweet = status.extended_tweet["full_text"]
    	except AttributeError:
    		tweet = status.text

    	with self.lock:
    		self.batchedtweets.append((status.user.screen_name, tweet))
    	return True

    def on_error(self, status_code):
        if status_code == 420:
            return False


class TwitterRobotPushManager:
	def __init__(self):
		self.batchedtweets = []
		self.lock = Lock()
		self.emotion_dict = {"Happy" : 'h', "Angry" : 'a', "Excited" : 'e', "Sad" : 's', "Sarcasm" : 'u', "Fear" : 'f', "Bored" : 'b'}
		self.btport = serial.Serial(PORT, BAUD_RATE)

	def getemotion(self, tweet):
		emotion= (paralleldots.emotion(tweet)['emotion'])['emotion']
		return self.emotion_dict[emotion]

	def sendtweetdata(self, handle, tweet, emotion):
		tweet = tweet.replace(" ", "-")
		print("@"+handle+" says:")
		print(tweet)
		print(emotion)
		tweet_chunks = [tweet[i:i+CHUNK_SIZE] for i in range(0, len(tweet), CHUNK_SIZE)]
		print ("TWEET CHUNKS")
		for i in range (len (tweet_chunks)):
			print (tweet_chunks[i])
		self.btport.write((handle+TERM_CHAR).encode())
		for i in range(0, len(tweet_chunks)):
			self.btport.write((tweet_chunks[i]+TERM_CHAR).encode())
		for i in range(len(tweet_chunks), 5):
			self.btport.write((EMPTY_CHUNK+TERM_CHAR).encode())
		self.btport.write((emotion+TERM_CHAR).encode())
		print ("sending tweet data")

	def processtweets(self):
		updateflag = False
		while (1):
			with self.lock:
				if (len(self.batchedtweets) > 0):
					handle, tweet = self.batchedtweets.pop(0)
					updateflag = True
			if (updateflag):
				updateflag = False
				emotion = self.getemotion(tweet)
				self.sendtweetdata(handle, tweet, emotion)
				time.sleep(EXECUTION_TIME)

	def tweetfilter(self, stream):
		stream.filter(track=[BOT_HANDLE])

	def start(self):
		paralleldots.set_api_key(paralleldots_key)
		listener = StreamListener(self.batchedtweets, self.lock)
		auth = OAuthHandler(consumer_key, consumer_secret)
		auth.set_access_token(access_token, access_token_secret)
		stream = Stream(auth, listener)
		Thread(target = self.tweetfilter, args = (stream,)).start()
		thread = Thread(target = self.processtweets)
		thread.start()
		print("Started...")


if __name__ == '__main__':
    botpushmanager = TwitterRobotPushManager()
    botpushmanager.start()
