# 2020/6/15 @jiaheng    A script to make test scenarios for the fast radix tree project.
# It would read people's literary works (e.g. Shakespeare's works), doing data-cleaning according to text technologies
# (removing symbols, removing stop words and stemming etc.).


# Shakespeare's works: http://shakespeare.mit.edu/

import file_reader as r

if __name__ == '__main__':
    doc = r.FileReader("t8.shakespeare")

    doc.display()
