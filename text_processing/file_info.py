# 2020/6/15 @jiaheng    A class describes a particular text file.
# It's responsible for pre-processing (data cleaning) and indexing.

# 2020/6.16 @jiaheng    Consider NOT to apply stemming while making the index for fast radix tree test scenario.


import re

from string import punctuation
from collections import defaultdict

from nltk.stem.snowball import SnowballStemmer

stop_words = open('englishST.txt',
                  'r+').read().split()  # https://www.scrapmaker.com/data/wordlists/unorganized/englishST.txt


class FileInfo:
    def __init__(self):
        self.line_cnt = 0
        self.word_cnt = 0

        self.dic = defaultdict(int)  # dict<str, int>

    def read(self, s, affix=False):
        """
        Read the text file line by line, doing pre-processing (stemming).
        """
        # TODO(jiaheng): Do data-cleaning (symbols -> stop-words -> affix).
        tokens = FileInfo.remove_affix(s) if affix else FileInfo.remove_stop_words(s)

        # examine the list
        for token in tokens:
            self.dic[token] += 1

        # update count numbers
        self.line_cnt += 1
        self.word_cnt += len(tokens)

    def save(self, fn):
        """
        Save the result (got so far) into an output file named by `fn`.
        """
        # sort the dict (as a list) high freq -> low
        tokens = sorted(self.dic.items(), key=lambda d: d[1], reverse=True)

        try:
            with open(fn, 'w') as f:
                for ele in tokens:
                    f.write(str(ele[0]) + '\t' + str(ele[1]) + '\n')
        except IOError:
            pass

    def display(self, bound=50):
        """
        Print the first i (default = 50) records in the dictionary.
        """
        # able to know the total number of lines
        print("lines:", self.line_cnt, "words:", self.word_cnt)

        # check the first i records in the list
        tokens = sorted(self.dic.items(), key=lambda d: d[1], reverse=True)
        for i in range(bound):
            if i < len(tokens):
                print(tokens[i])
            else:
                break

    @staticmethod
    def remove_symbols(s):
        """
        Receive a string (one line in the doc), and remove all symbols within it.
        """
        # split
        s = ''.join([c for c in s.lower() if c not in punctuation])

        return re.sub(' +', ' ', s)

    @staticmethod
    def remove_stop_words(s):
        """
        Remove stop words indicated by: https://www.scrapmaker.com/data/wordlists/unorganized/englishST.txt.
        """
        all_token = [
            token for token in re.findall("[A-Za-z0-9]+", FileInfo.remove_symbols(s))  # need to firstly remove symbols
        ]

        return [
            token for token in all_token if token not in stop_words  # remove stop-words
        ]

    @staticmethod
    def remove_affix(s):
        """
        Rebuild the string as a token list then remove affix.
        """
        ps = SnowballStemmer("english")

        return [
            ps.stem(token) for token in FileInfo.remove_stop_words(s)
        ]
