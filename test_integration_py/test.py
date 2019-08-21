
import time
from furl import furl
from vlc import * # bad practice

logging.basicConfig(level=logging.DEBUG)

class TestPlayer(object):
	echo_position = False

	def __init__(self, url, vlc_arguments):
		self.url               = url
		self.vlc_arguments     = vlc_arguments

	def decoded_time(self):
		return self.player.get_time() / 1000.0

	def test_source(self, max_play_duration):
		self.max_play_duration = max_play_duration
		self.play()
		# state transitioning: State.NothingSpecial > State.Opening > State.Playing > State.Stopped
		# if error in openning state will be State.Ended
		while self.player.get_state() not in (State.Ended, State.Stopped):
			time.sleep(0.3)
			if self.decoded_time() > self.max_play_duration:
				break
		result = self.decoded_time() > self.max_play_duration
		self.player.stop()
		# clean the resources
		self.player.release()
		self.instance.release()
		return result

	def play(self):
		self.instance  = Instance(["--sub-source=marq"] + self.vlc_arguments)
		try:
			media = self.instance.media_new(self.url)
		except (AttributeError, NameError) as e:
			print('%s: %s (%s %s vs LibVLC %s)' % (
				e.__class__.__name__, e,
				sys.argv[0], __version__,
				"unk"
				)
			)
			raise

		self.player = self.instance.media_player_new()
		self.player.set_media(media)
		self.player.play()

		self.player.video_set_marquee_int(VideoMarqueeOption.Enable, 1)
		self.player.video_set_marquee_int(VideoMarqueeOption.Size, 24)  # pixels
		self.player.video_set_marquee_int(VideoMarqueeOption.Position, Position.Bottom)
		self.player.video_set_marquee_int(VideoMarqueeOption.Timeout, 0)  # millisec, 0==forever
		self.player.video_set_marquee_int(VideoMarqueeOption.Refresh, 1000)  # millisec (or sec?)
		self.player.video_set_marquee_string(VideoMarqueeOption.Text, str_to_bytes('%Y-%m-%d  %H:%M:%S'))

class TestOutcome:
	INVALID_URL = "INVALID_URL"
	PASSING     = "PASSING"
	FAILED      = "FAILED"
	def __init__(self, url, outcome):
		self.url, self.outcome = url, outcome
	def __str__(self):
		return "[TestOutcome {} {}]".format(self.outcome, self.url)
	__repr__ = __str__

class Configuration:
	direct_url_decoding_time = 0.6
	proxy_decoding_time      = 6.0

def test_proxy_server(hls_url, proxy_domain, proxy_port):
	proxy_url        = furl(hls_url)
	original_scheme  = proxy_url.scheme
	original_host    = proxy_url.host
	original_port    = proxy_url.port
	proxy_url.scheme = "http"
	proxy_url.host   = proxy_domain
	proxy_url.port   = str(proxy_port)
	ssl = "1" if original_scheme == "https" else "0"
	proxy_url.path.segments = ["{}~{}~{}".format(ssl, original_port, original_host)] + proxy_url.path.segments
	print "original url", hls_url
	print "proxy url   ", proxy_url.url
	
	# test hls_url directly
	if not TestPlayer(hls_url, []).test_source(Configuration.direct_url_decoding_time):
		return TestOutcome(hls_url, TestOutcome.INVALID_URL)
	# test proxy operation
	if TestPlayer(proxy_url.url, []).test_source(Configuration.proxy_decoding_time):
		return TestOutcome(hls_url, TestOutcome.PASSING)
	return TestOutcome(hls_url, TestOutcome.FAILED)

def run_integration_test(hls_sources_file_path, proxy_domain, proxy_port):
	results = []
	for line in open(hls_sources_file_path):
		hls_url = line.strip()
		if len(hls_url) > 7:
			test_outcome = test_proxy_server(hls_url, proxy_domain, proxy_port)
			print test_outcome
			results.append(test_outcome)
	print "Test run complete:"
	for result in results:
		print result

if __name__ == '__main__':
	if len(sys.argv) < 4:
		print "usage: python test.py hls_sources.txt proxy-server-host proxy-server-port"
		sys.exit()

	hls_sources_file_path = sys.argv[1]
	proxy_domain          = sys.argv[2]
	proxy_port            = sys.argv[3]
	run_integration_test(hls_sources_file_path, proxy_domain, proxy_port)
