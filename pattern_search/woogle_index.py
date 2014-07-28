#!/usr/bin/env python3

import	getopt
import	sys
import	re
import	random
import	sqlite3

def main():
	g_input_fn = False
	g_do_search = False
	g_dict_fn = False
	g_words = []

	try:
		opts, args = getopt.getopt(sys.argv[1:],
				"hi:d:s:v",
				["help", "input=", "dict=", "search="])
	except getopt.GetoptError as err:
		print(str(err))
		usage()
		sys.exit(2)
	output = None
	verbose = False
	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit()
		elif o in ("-i", "--input"):
			g_input_fn = a
		elif o in ("-s", "--search"):
			g_do_search = a;
		elif o in ("-d", "--dict"):
			g_dict_fn = a
		else:
			assert False, "unhandled option"

	if g_input_fn == False:
		print("You must pass --input to indicate where DB is");
		sys.exit(2);
	if g_do_search == False and g_dict_fn == False:
		print("You must either pass --input with --dict");
		sys.exit(2)

	conn = sqlite3.connect(g_input_fn);
	c = conn.cursor()
	if g_do_search == False:
		assert(g_dict_fn != None);
		print("# initializing database " + g_dict_fn);
		with open(g_dict_fn, "r") as f:
			g_words += [ [line, random.randint(0, 1000)]
				for line in f.read().split("\n")
					if not re.match("^$", line)]
		f.close()

		c.execute("DROP TABLE IF EXISTS words");
		c.execute('''CREATE TABLE words(word text, score real)''')
		for word in g_words:
			if len(word) <= 0:
				continue;
			c.execute("""
				INSERT INTO words VALUES('{0}','{1}');
				""".format(word[0], word[1]));
		conn.commit();
		conn.close();
	else:
		# From http://stackoverflow.com/questions/5071601/how-do-i-use-regex-in-a-sqlite-query
		def match(expr, item):
			return re.match(expr, item) is not None

		conn.create_function("MATCH", 2, match)
		c.execute("""
				SELECT * FROM words
				WHERE MATCH('.*{0}.*', word)
				ORDER BY score DESC LIMIT 10;
			""".format(g_do_search));

		for v, r in c.fetchall():
			print(v, r)



if __name__ == "__main__":
    main()
