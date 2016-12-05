#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <math.h>
#include <set>
#include <iterator>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <map>
#include "libstemmer_c/include/libstemmer.h"

using namespace std;

static int pretty = 3;

char * stop_dic_file = "common-english-words.dic";
char * doc_list_file = "list.txt";
char * sufix = "_stemmed";
char * language = "english";
char * charenc = "ISO_8859_1";

static void stem_file(struct sb_stemmer * stemmer, FILE * f_in, FILE * f_out) {
#define INC 10
	int lim = INC;
	sb_symbol * b = (sb_symbol *) malloc(lim * sizeof(sb_symbol));

	while (1) {
		int ch = getc(f_in);
		if (ch == EOF) {
			free(b);
			return;
		}
		{

			int i = 0;
			int inlen = 0;
			while (1) {
				if (ch == '\n' || ch == EOF)
					break;
				if ((ch == ',') || (ch == '(') || (ch == ')') || (ch == '!') || (ch == '?') || (ch == '!') ||
								  (ch == '$') || (int(ch)==39) || (ch == '"') || (ch == '.') ||
								  (ch == ':') || (ch == '-') || (ch == '/') ){
							ch = ' ';
							}
				if (i == lim) {
					sb_symbol * newb;
					newb = (sb_symbol *) realloc(b,
							(lim + INC) * sizeof(sb_symbol));
					if (newb == 0)
						goto error;
					b = newb;
					lim = lim + INC;
				}
				/* Update count of utf-8 characters. */
				if (ch < 0x80 || ch > 0xBF)
					inlen += 1;
				/* force lower case: */
				if (isupper(ch))
					ch = tolower(ch);

				b[i] = ch;
				i++;
				ch = getc(f_in);
			}

			{
				const sb_symbol * stemmed = sb_stemmer_stem(stemmer, b, i);
				if (stemmed == NULL) {
					fprintf(stderr, "Out of memory");
					exit(1);
				} else {
					if (pretty == 1) {
						fwrite(b, i, 1, f_out);
						fputs(" -> ", f_out);
					} else if (pretty == 2) {
						fwrite(b, i, 1, f_out);
						if (sb_stemmer_length(stemmer) > 0) {
							int j;
							if (inlen < 30) {
								for (j = 30 - inlen; j > 0; j--)
									fputs(" ", f_out);
							} else {
								fputs("\n", f_out);
								for (j = 30; j > 0; j--)
									fputs(" ", f_out);
							}
						}
					}

					fputs((char *) stemmed, f_out);
					putc('\n', f_out);
				}
			}
		}
	}
	error: if (b != 0)
		free(b);
	return;
}

void stemm_doc(string str) {
	struct sb_stemmer * stemmer;
	char * out = new char[256];
	out[0] = 0;
	strcat(out, "");
	memcpy(out, str.c_str(), 100*sizeof(char));
	strcat(out, sufix);
	FILE * f_in;
	FILE * f_out;
	f_in = (str.c_str() == 0) ? stdin : fopen(str.c_str(), "r");
	if (f_in == 0) {
		fprintf(stderr, "file %s not found\n", str.c_str());
		exit(1);
	}
	f_out = (out == 0) ? stdout : fopen(out, "w");
	if (f_out == 0) {
		fprintf(stderr, "file %s cannot be opened\n", out);
		exit(1);
	}
	stemmer = sb_stemmer_new(language, charenc);
	stem_file(stemmer, f_in, f_out);
	sb_stemmer_delete(stemmer);
	if (str.c_str() != 0)
		(void) fclose(f_in);
	if (out != 0)
		(void) fclose(f_out);
	delete[] out;
}

void dicParse(char * in, set<string> &stop_dic) {
	stemm_doc(in);
	char * out = new char[256];
	out[0] = 0;

	strcat(out, "");
	memcpy(out, in, 20*sizeof(in));
	strcat(out, sufix);
	ifstream stemmed(out);
	set<string> result;
	while (!stemmed.eof()) {
		string word;
		stemmed >> word;
		stop_dic.insert(word);
	}
	delete[] out;
	stemmed.close();
}

void doc_list_parse(char * doc_name, vector<string> &doc_lists) {
	ifstream in(doc_name);
	string line;
	while (getline(in, line)) {
		stemm_doc(line);
		doc_lists.push_back(line + sufix);
	}
	in.close();
}

void clean_stop_words(set<string> &dic, vector<string> &docs) {
	for (unsigned int i = 0; i < docs.size(); i++) {
		ifstream in(docs[i].c_str());
		docs[i] += "_clean";
		ofstream out(docs[i].c_str());
		string t;
		while (in >> t) {

			if (dic.find(t) == dic.end()) {
				out << t << " ";
			}
		}
		in.close();
		out.close();
	}
}

void load_maps(vector<string> &docs, vector<map<string, int> > &one_gram,
		vector<map<string, int> > &bi_gram,
		vector<map<string, int> > &three_gram) {
	for (unsigned int i = 0; i < docs.size(); i++) {
		ifstream in(docs[i].c_str());
		string pre_pre, prev, cur;

		in >> pre_pre;
		one_gram[i].insert(pair<string, int>(pre_pre, 1));

		in >> prev;
		if (one_gram[i].find(prev) != one_gram[i].end()) {
			one_gram[i].at(prev)++;}
else		one_gram[i].insert(pair<string,int>(prev,1));
		bi_gram[i].insert(pair<string, int>(pre_pre + " " + prev, 1));

		in >> cur;

		if (one_gram[i].find(cur) != one_gram[i].end()) {
			one_gram[i].at(cur)++;}
else		one_gram[i].insert(pair<string,int>(cur,1));

		if (bi_gram[i].find(prev + " " + cur) != bi_gram[i].end()) {
			bi_gram[i].at(prev + " " + cur)++;}
else		bi_gram[i].insert(pair<string,int>(prev+ " " + cur,1));

		three_gram[i].insert(
				pair<string, int>(pre_pre + " " + prev + " " + cur, 1));

		while (!(in.eof())) {
			pre_pre = prev;
			prev = cur;
			in >> cur;

			if (one_gram[i].find(cur) != one_gram[i].end()) {
				one_gram[i].at(cur)++;}
else			one_gram[i].insert(pair<string,int>(cur,1));

			if (bi_gram[i].find(prev + " " + cur) != bi_gram[i].end()) {
				bi_gram[i].at(prev + " " + cur)++;}
else			bi_gram[i].insert(pair<string,int>(prev+ " " + cur,1));

			if (three_gram[i].find(pre_pre + " " + prev + " " + cur)
					!= three_gram[i].end()) {
				three_gram[i].at(pre_pre + " " + prev + " " + cur)++;}
else			three_gram[i].insert(pair<string,int>(pre_pre + " " + prev + " " + cur,1));

		}
		in.close();
	}
}

void find_all_tfidf(vector< vector<double> > &tfidf, vector< map<string, int> > &gram)
{
	for (unsigned i=0;i<tfidf.size();i++){
		for( map<string, int>::iterator it=gram[i].begin(); it!=gram[i].end(); it++){
			int count_docs=0;
			for(unsigned j=0;j<tfidf.size();j++)
			{
				if (i!=j) {
					if (gram[j].find(it->first)!=gram[j].end()){
						count_docs++;
					}
				}

			}
			tfidf[i].push_back((double)it->second/(double)gram[i].size() * log((double)gram.size() / (double)count_docs));
		}
	}

}

void printTFIDF(vector<string> &docs, vector<map<string, int> > &gram, vector< vector<double> > &tfidf)
{
	for (unsigned i=0;i<tfidf.size();i++){
		string t = docs[i];
		//t.erase(t.end()-20,t.end());
		t+="_tfidf";
		ofstream out(t.c_str());
		int j=0;
		for( map<string, int>::iterator it=gram[i].begin(); it!=gram[i].end(); it++){
				out<< it->first << " " << tfidf[i][j] << endl;
				j++;
		}
		out.close();
	}
}

void printGRAPH(vector<string> &docs, vector<map<string, int> > &gram, vector< vector<double> > &tfidf)
{

	for (unsigned i=0;i<tfidf.size();i++){
		string s,t = docs[i];
		vector< pair<string,double> > res;
		//t.erase(t.end()-20,t.end());
		s=t;
		s+="_graph_v.csv";
		t+="_graph.csv";
		ofstream out(t.c_str());
		ofstream out_v(s.c_str());
		out<<"Source,Target,Weight"<<endl;
		out_v<<"Id,Label"<<endl;
		int j=0;
		for( map<string, int>::iterator it=gram[i].begin(); it!=gram[i].end(); it++){
				res.push_back(pair<string,double>(it->first,tfidf[i][j]));
				j++;
		}
		for (int k=0;k<res.size();k++)
			for( int m=0;m<res.size();m++)
			{
				if (res[m].second < res[k].second)
				{
					pair<string,double> tmp;
					tmp = res[m];
					res[m] = res[k];
					res[k] = tmp;
				}
			}
		for(int k=1;k<res.size();k++){
			if(res[k-1].second<100)
			{
				out<<res[k-1].first<<","<<res[k].first<<","<<res[k-1].second<<endl;
				out_v<<res[k-1].first<<","<<res[k-1].first<<endl;
			}
		}
		out.close();
		out_v.close();
	}
}

int main() {
	set<string> stop_dic_set;
	dicParse(stop_dic_file, stop_dic_set);
	vector<string> stemmed_docs;
	doc_list_parse(doc_list_file, stemmed_docs);
	clean_stop_words(stop_dic_set, stemmed_docs);
	vector<map<string, int> > one_gram(stemmed_docs.size());
	vector<map<string, int> > bi_gram(stemmed_docs.size());
	vector<map<string, int> > three_gram(stemmed_docs.size());
	load_maps(stemmed_docs, one_gram, bi_gram, three_gram);
	vector< vector<double> > tfidf_one_gram(stemmed_docs.size());
	vector< vector<double> > tfidf_bi_gram(stemmed_docs.size());
	vector< vector<double> > tfidf_three_gram(stemmed_docs.size());
	find_all_tfidf(tfidf_one_gram,one_gram);
	find_all_tfidf(tfidf_bi_gram,bi_gram);
	find_all_tfidf(tfidf_three_gram,three_gram);
	//printTFIDF(stemmed_docs,one_gram,tfidf_one_gram);
	printGRAPH(stemmed_docs,three_gram,tfidf_three_gram);
	return 0;
}
