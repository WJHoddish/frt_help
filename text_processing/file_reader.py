# 2020/6/15 @jiaheng    A file reader class which takes a file path, do pre-processing and store all useful information.


from file_info import FileInfo


class FileReader:
    def __init__(self, fn):
        """
        Store a file name.
        """
        self._fn = fn
        self._info = None

        self.__parse()

    def __parse(self):
        """
        Each call will renew the file info, private function.
        """
        try:
            with open(self._fn + '.txt', 'r') as f:
                # TODO(jiaheng): (re)Set information.
                self._info = FileInfo()

                # TODO(jiaheng): Process each line in the text, add raw text into the document.
                for line in f:
                    self._info.read(line)
                    # break  # test one line

                self._info.save(self._fn + '.result' + '.txt')
        except IOError:
            pass

    def display(self):
        """
        Present all information about the corpus.
        """
        if self._info:
            # if the file has been successfully parsed, present
            self._info.display()
