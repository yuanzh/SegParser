import java.io.*;
import java.util.*;

class MadaLine {
	public double score;
	public String segStr;		// with +
	public String wordStr;		// without +
	public String[] seg;
	public String[] pos;
	public int detIndex;
	public String lemma;
	public String per;
	public String gen;
	public String num;
	public int morphIndex;
	
	public MadaLine() {
		score = 0.0;
		detIndex = -1;	// no Al_det
		morphIndex = -1;
		per = "na";
		gen = "na";
		num = "na";
	}
}

class MadaWord {
	String word;
	MadaLine[] lines;
}

class MadaSentence {
	MadaWord[] words;
}

public class MadaReader {

	public BufferedReader reader;
	public HashMap<String, String> fine2coarse;
	public HashMap<String, String> coarse2core12;
	
	public String segmentScheme = "catib";		// catib:spmrl, qatar:qatar
	
	public int maxLine = 1;
	
	public static final String pronSuffix[] = {
		"kmA", "hmA", "nA", "ny", "km", "hm", "hA", "hn", "kn", "y", "k", "h" 
	};
	
	public static final String suffix[] = {
		"kmA", "hmA", "nA", "ny", "km", "hm", "hA", "hn", "kn", "y", "k", "h", 
		"yn", "wn", "wA", "At", "A", "t", "n", "p"
	};
	
	public static final String[][] encMATable = { {"mA_sub", "mA", "CONJ"},
			{"ma_sub", "m", "CONJ"}, {"mA_rel", "mA", "REL"}, {"ma_rel", "m", "REL"}, {"man_rel", "mn", "REL"},
			{"man_interrog", "mn", "PART"}, {"mA_interrog", "mA", "PART"}, {"ma_interrog", "m", "PART"},
			{"lA_neg", "lA", "PART"}};

	public MadaReader() throws IOException {
		loadTagMap();
	}
	
	public void open(String file) throws IOException {
		reader = new BufferedReader(new FileReader(file));
	}
	
	public void close() throws IOException {
		if (reader != null)
			reader.close();
	}
	
	public void loadTagMap() throws IOException {
		fine2coarse = new HashMap<String, String>();
		
		BufferedReader br = new BufferedReader(new FileReader("../data/qatar/tags-all.mod.txt"));
		String str = null;
		while ((str = br.readLine()) != null) {
			String[] data = str.split("\\s+");
			if (data.length > 1)
				fine2coarse.put(data[0], data[1]);
			else
				fine2coarse.put(data[0], data[0]);
		}
		br.close();
		
		coarse2core12 = new HashMap<String, String>();
		
		br = new BufferedReader(new FileReader("../data/qatar/tags-mada2core12.txt"));
		while ((str = br.readLine()) != null) {
			String[] data = str.split("\\s+");
			coarse2core12.put(data[0], data[1]);
		}
		br.close();
	}
	
	public void addToList(ArrayList<String> wordList, ArrayList<String> posList,
			String word, String pos) {
		if (word.isEmpty()) {
			return;
		}
		else {
			Assert(!pos.isEmpty());
		}
		
		int size = wordList.size();
		if (size > 0 && posList.get(size - 1).equals("DET")) {
			Assert(wordList.get(size - 1).equals("Al"));
			if (!(pos.equals("NOUN") || pos.equals("ADJ") || pos.equals("NUM")  
					|| pos.equals("ABBREV") || pos.equals("NOUN_PROP") || pos.equals("FOREIGN")))
				System.out.println(pos);
			Assert(pos.equals("NOUN") || pos.equals("ADJ") || pos.equals("NUM")  
					|| pos.equals("ABBREV") || pos.equals("NOUN_PROP") || pos.equals("FOREIGN"));
		
			wordList.set(size - 1, wordList.get(size - 1) + word);
			posList.set(size - 1, pos);
		}
		else if (pos.equals("NSUFF")) {
			Assert(size > 0);
			wordList.set(size - 1, wordList.get(size - 1) + word);
		}
		else if (pos.equals("VSUFF")) {
			Assert(size > 0);
			wordList.set(size - 1, wordList.get(size - 1) + word);
		}
		else if (pos.equals("CASE")) {
			Assert(size > 0);
			wordList.set(size - 1, wordList.get(size - 1) + word);
		}
		else if (size > 0 && pos.equals(posList.get(size - 1))) {
			if (!(pos.equals("V") || pos.equals("CONJ") || pos.equals("PREP")
					|| pos.equals("PART"))) {
				for (int i = 0; i < wordList.size(); ++i)
					System.out.println(wordList.get(i) + "/" + posList.get(i));
				System.out.println(word + "/" + pos);
			}
			Assert(pos.equals("V") || pos.equals("CONJ") || pos.equals("PREP")
					|| pos.equals("PART"));
			if (pos.equals("V")) {
				wordList.set(size - 1, wordList.get(size - 1) + word);
			}
			else {
				wordList.add(word);
				posList.add(pos);
			}
		}
		else {
			wordList.add(word);
			posList.add(pos);
		}
	}
	
	public void findAldetIndex(MadaLine line) {
		int index = -1;
		for (int i = 0; i < line.pos.length; ++i) {
			String pos = line.pos[i];
			if (pos.equals("N") || pos.equals("AJ") || pos.equals("ABBREV") || pos.equals("PN")) {
				Assert(index == -1);
				index = i;
			}
		}
		Assert(index != -1);
		line.detIndex = index;
	}
	
	public int[] getPosPriority(MadaLine line) {
		int[] p = new int[line.pos.length];
		
		for (int i = 0; i < line.pos.length; ++i) {
			if (line.pos[i].equals("N")) {
				p[i] = 4;
			}
			else if (line.pos[i].equals("PN")
					|| line.pos[i].equals("ABBREV")
					|| line.pos[i].equals("AJ")) {
				p[i] = 3;
			}
			else if (line.pos[i].equals("V")) {
				p[i] = 2;
			}
			else if (line.pos[i].equals("PRO")
						|| line.pos[i].equals("AV")
						|| line.pos[i].equals("REL")) {
				p[i] = 1;
			}
			else {
				p[i] = 0;
			}
		}
		
		return p;
	}

	public void processMorphologyData(MadaLine line, String[] data) {
		int[] priority = getPosPriority(line);
		int highest = -1;
		for (int i = 0; i < priority.length; ++i) {
			if (priority[i] > highest) {
				highest = priority[i];
				line.morphIndex = i;
			}
		}
		// lemma
		Assert(data[2].startsWith("lex"));
		line.lemma = data[2].substring(4, data[2].lastIndexOf("_"));
		line.lemma = normalizeForm(line.lemma, line.pos[line.morphIndex]);
		line.lemma = normalizeAlif(line.lemma);
		Assert(!line.lemma.isEmpty());
		
		// det
		Assert(data[9].startsWith("prc0"));
		if (data[9].split(":")[1].equals("Al_det")) {
			findAldetIndex(line);
		}
		
		// per, gen, num
		Assert(data[10].startsWith("per"));
		line.per = data[10].split(":")[1];
		Assert(data[14].startsWith("gen"));
		line.gen = data[14].split(":")[1];
		Assert(data[15].startsWith("num"));
		line.num = data[15].split(":")[1];
	}
	
	public void concatSegStr(MadaLine line) {
		line.segStr = "";
		line.wordStr = "";
		for (int i = 0; i < line.seg.length; ++i) {
			if (i > 0)
				line.segStr += "+";
			line.segStr += line.seg[i];
			line.wordStr += line.seg[i];
		}
	}
	
	public MadaLine readNextLine() throws IOException {
		MadaLine line = new MadaLine();
		String str = reader.readLine();
		
		if (str.equals("--------------"))
			return null;
		
		String[] data = str.split(" ");
		Assert(data[0].charAt(0) == '*' || data[0].charAt(0) == '_' || data[0].charAt(0) == '^');
		
		line.score = Double.parseDouble(data[0].substring(1));
		
		Assert(data[3].startsWith("bw:"));
		data[3] = data[3].substring(3);
		
		String[] bwData = data[3].split("\\+");
		ArrayList<String> tmpWord = new ArrayList<String>();
		ArrayList<String> tmpPos = new ArrayList<String>();
		for (int i = 0; i < bwData.length; ++i) {
			if (bwData[i].isEmpty())
				continue;
			
			int split = bwData[i].lastIndexOf("/");
			Assert(split > 0);
			
			String posStr = bwData[i].substring(split + 1);
			String wordStr = bwData[i].substring(0, split);

			String pos = normalizePos(posStr);
			String word = normalizeForm(wordStr, pos);
			
			// fix
			if (word.equals("\"") || word.equals("_"))
				pos = "PUNC";
			
			if (segmentScheme.equals("catib")) {
				addToList(tmpWord, tmpPos, word, pos);
			}
			else {
				if (!word.isEmpty()) {
					Assert(!pos.isEmpty());
					tmpWord.add(word);
					tmpPos.add(pos);
				}
			}
			
		}
		
		line.seg = new String[tmpWord.size()];
		line.seg = tmpWord.toArray(line.seg);
		line.pos = new String[tmpPos.size()];
		line.pos = tmpPos.toArray(line.pos);
		
		Assert(line.seg.length > 0);

		if (segmentScheme.equals("catib")) {
			for (int i = 0; i < line.pos.length; ++i) {
				line.pos[i] = convertToCore12(line.pos[i]);
			}
			processMorphologyData(line, data);
		}
		

		concatSegStr(line);
		Assert(!line.segStr.isEmpty());
		
		return line;
	}
	
	public MadaLine processSVMPrediction(String str) {
		MadaLine line = new MadaLine();
		String[] data = str.split("\\s+");
		
		ArrayList<String> tmpWord = new ArrayList<String>();
		ArrayList<String> tmpPos = new ArrayList<String>();
		
		line.lemma = data[1];

		// per, gen, num
		Assert(data[5].startsWith("gen"));
		line.gen = data[5].split(":")[1];
		Assert(data[7].startsWith("num"));
		line.num = data[7].split(":")[1];
		Assert(data[8].startsWith("per"));
		line.per = data[8].split(":")[1];
		
		// morphology
		Assert(data[13].startsWith("prc3:"));
		Assert(data[12].startsWith("prc2:"));
		Assert(data[11].startsWith("prc1:"));
		Assert(data[10].startsWith("prc0:"));

		String prc3 = data[13].split(":")[1];
		String prc2 = data[12].split(":")[1];
		String prc1 = data[11].split(":")[1];
		String prc0 = data[10].split(":")[1];
		
		if (prc3.equals("0")) {
			// do nothing
		}
		else {
			System.out.println("prc3: " + prc3);
		}
		
		if (prc2.equals("0")) {
			// do nothing
		}
		else if (prc2.equals("wa_conj") || prc2.equals("fa_conj")) {
			String s = prc2.substring(0, 1);
			tmpWord.add(s);
			tmpPos.add("CONJ");
			if (line.lemma.charAt(0) == prc2.charAt(0))
				line.lemma = line.lemma.substring(1);
		}
		else if (prc2.equals("wa_prep")) {
			tmpWord.add("w");
			tmpPos.add("PREP");
			if (line.lemma.charAt(0) == 'w')
				line.lemma = line.lemma.substring(1);
		}
		else if (prc2.equals("fa_conn")) {
			tmpWord.add("f");
			tmpPos.add("PART");
			if (line.lemma.charAt(0) == 'w')
				line.lemma = line.lemma.substring(1);
		}
		else {
			System.out.println("prc2: " + prc2);
		}
		
		if (prc1.equals("0")) {
			// do nothing
		}
		else if (prc1.equals("li_prep") || prc1.equals("ta_prep") || prc1.equals("bi_prep")
				|| prc1.equals("la_prep") || prc1.equals("ka_prep") || prc1.equals("wa_prep")) {
			String s = prc1.substring(0, 1);
			tmpWord.add(s);
			tmpPos.add("PREP");
			if (line.lemma.charAt(0) == prc1.charAt(0))
				line.lemma = line.lemma.substring(1);
		}
		else if (prc1.equals("la_rc") || prc1.equals("bi_part") || prc1.equals("sa_fut") || prc1.equals("la_emph") || prc1.equals("li_jus")) {
			String s = prc1.substring(0, 1);
			tmpWord.add(s);
			tmpPos.add("PART");
			if (line.lemma.charAt(0) == prc1.charAt(0))
				line.lemma = line.lemma.substring(1);
		}
		else {
			System.out.println("prc1: " + prc1);
		}
		
		// pos and main word
		String word = line.lemma;
		tmpWord.add(word);
		Assert(data[9].startsWith("pos:"));
		tmpPos.add(data[9].split(":")[1]);
		line.morphIndex = tmpWord.size() - 1;
		
		if (prc0.equals("0") || prc0.equals("na")) {
			// do nothing
		}
		else if (prc0.equals("Al_det")) {
			if (line.lemma.startsWith("Al"))
				line.lemma = line.lemma.substring(2);
			line.detIndex = line.morphIndex;
		}
		else {
			System.out.println("prc0: " + prc0);
		}
		
		Assert(data[4].startsWith("enc0:"));
		String enc = data[4].split(":")[1];
		if (enc.equals("0")) {
			// do nothing
		}
		else {
			boolean find = false;
			for (int i = 0; i < encMATable.length; ++i)
				if (enc.equals(encMATable[i][0])) {
					tmpWord.add(encMATable[i][1]);
					tmpPos.add(encMATable[i][2]);
					if (word.endsWith(encMATable[i][1])) {
						word = word.substring(0, word.length() - encMATable[i][1].length());
						line.lemma = line.lemma.substring(0, line.lemma.length() - encMATable[i][1].length());
					}
					find = true;
					break;
				}
			if (!find) {
				for (int i = 0; i < pronSuffix.length; ++i) {
					if (word.endsWith(pronSuffix[i])) {
						tmpWord.add(pronSuffix[i]);
						tmpPos.add("PRON");
						word = word.substring(0, word.length() - pronSuffix[i].length());
						line.lemma = line.lemma.substring(0, line.lemma.length() - pronSuffix[i].length());
						break;
					}
				}
			}
		}
		
		// remove noun suffix from lemma
		for (int i = 0; i < suffix.length; ++i) {
			if (line.lemma.endsWith(suffix[i]) && !line.lemma.equals(suffix[i])) {
				line.lemma = line.lemma.substring(0, line.lemma.length() - suffix[i].length());
				break;
			}
		}
		
		line.lemma = normalizeAlif(line.lemma);

		Assert(!word.isEmpty());
		Assert(!line.lemma.isEmpty());
		tmpWord.set(line.morphIndex, word);
		
		line.seg = new String[tmpWord.size()];
		line.seg = tmpWord.toArray(line.seg);
		line.pos = new String[tmpPos.size()];
		line.pos = tmpPos.toArray(line.pos);
		
		Assert(line.seg.length > 0);

		if (segmentScheme.equals("catib")) {
			for (int i = 0; i < line.pos.length; ++i) {
				line.pos[i] = convertToCore12(line.pos[i]);
			}
		}
		
		concatSegStr(line);
		Assert(!line.segStr.isEmpty());
		
		return line;
	}
	
	public boolean fixWordMismatch(MadaLine line, String word) {
		if (line.wordStr.length() != word.length()) {
			// length mismatch, cannot fix
			return false;
		}
		
		// replace segments based on length
		int st = 0;
		for (int i = 0; i < line.seg.length; ++i) {
			int en = st + line.seg[i].length();
			line.seg[i] = word.substring(st, en);
			st = en;
		}
		Assert(st == word.length());
		
		concatSegStr(line);
		return true;
	}
	
	public void hardFixWordMismatch(MadaLine line, String word) {
		System.out.println("Warning: hard fix " + word + " and " + line.wordStr);
		if (line.wordStr.indexOf(word) != -1) {
			// word is a substring
			int begin = line.wordStr.indexOf(word);
			int end = begin + word.length();
			
			int p = 0;
			for (int i = 0; i < line.seg.length; ++i) {
				int st = Math.max(p, begin);
				int en = Math.min(p + line.seg[i].length(), end);
				p += line.seg[i].length();
				line.seg[i] = st < en ? line.wordStr.substring(st, en) : "";
			}
			
			ArrayList<String> tmpSeg = new ArrayList<String>();
			ArrayList<String> tmpPos = new ArrayList<String>();
			
			int oldMorphIndex = line.morphIndex;
			int oldDetIndex = line.detIndex;

			for (int i = 0; i < line.seg.length; ++i) {
				if (line.seg[i].isEmpty()) {
					// remove
					if (i <= oldMorphIndex)
						line.morphIndex--;
					if (i <= oldDetIndex)
						line.detIndex--;
				}
				else {
					tmpSeg.add(line.seg[i]);
					tmpPos.add(line.pos[i]);
				}
			}
			if (line.morphIndex < 0) line.morphIndex = -1;
			if (line.detIndex < 0) line.detIndex = -1;
			
			line.seg = new String[tmpSeg.size()];
			line.pos = new String[tmpPos.size()];
			line.seg = tmpSeg.toArray(line.seg);
			line.pos = tmpPos.toArray(line.pos);
			
			concatSegStr(line);
			if (!line.wordStr.equals(word)) {
				System.out.println(word + " " + line.wordStr);
			}
			Assert(line.wordStr.equals(word));
		}
		// special cases
		else if (line.wordStr.substring(1, 3).equals("Al")
				&& word.equals(line.wordStr.substring(0, 1) + line.wordStr.substring(2))) {
			for (int i = 0; i < line.seg.length; ++i) {
				if (line.seg[i].startsWith("Al"))
					line.seg[i] = line.seg[i].substring(1);
			}
			concatSegStr(line);
			Assert(line.wordStr.equals(word));
		}
		else if ((line.wordStr.equals("lAl<lhA'") && word.equals("llAlhA'"))
				|| (line.wordStr.equals("lAl{stHSAl") && word.equals("llAstHSAl"))
				||(line.wordStr.equals("wlAl") && word.equals("wll"))) {
			for (int i = 0; i < line.seg.length; ++i) {
				if (line.seg[i].startsWith("Al"))
					line.seg[i] = line.seg[i].substring(1);
				if (line.seg[i].indexOf("<") != -1)
					line.seg[i] = line.seg[i].replaceAll("<", "A");
				if (line.seg[i].indexOf("{") != -1)
					line.seg[i] = line.seg[i].replaceAll("\\{", "A");
			}
			concatSegStr(line);
			Assert(line.wordStr.equals(word));
		}
		else if (line.wordStr.equals("lAl>$rf") && word.equals("ll<$rf")) {
			for (int i = 0; i < line.seg.length; ++i) {
				if (line.seg[i].startsWith("Al"))
					line.seg[i] = line.seg[i].substring(1);
				if (line.seg[i].indexOf(">") != -1)
					line.seg[i] = line.seg[i].replaceAll(">", "<");
			}
			concatSegStr(line);
			Assert(line.wordStr.equals(word));
		}
		// cannot handle
		else {
			System.out.println("Cannot fix " + word + " and " + line.wordStr);
			Assert(false);
		}
	}
	
	public MadaWord readNextWord() throws IOException {
		String head = ";;WORD ";
		String str = reader.readLine();
		if (str.equals("SENTENCE BREAK"))
			return null;
		
		Assert(str.startsWith(head));
		MadaWord word = new MadaWord();	
		word.word = str.substring(head.length());
		
		str = reader.readLine();
		if (str.equals(";;NO-ANALYSIS")) {
			str = reader.readLine();
			Assert(str.startsWith(";;SVM_PREDICTIONS: "));
			word.lines = new MadaLine[1];
			word.lines[0] = processSVMPrediction(str);
			
			if (!word.lines[0].wordStr.equals(word.word)) {
				if (!fixWordMismatch(word.lines[0], word.word)) {
					//System.out.println("Warning: cannot fix " + word.word + " and " + word.lines[0].wordStr);
					hardFixWordMismatch(word.lines[0], word.word);
				}
			}
			
			str = reader.readLine();
			str = reader.readLine();
			Assert(str.equals("--------------"));
		}
		else {
			Assert(str.startsWith(";;SVM_PREDICTIONS: "));
			ArrayList<MadaLine> tmpLines = new ArrayList<MadaLine>();
			ArrayList<MadaLine> tmpLinesFixed = new ArrayList<MadaLine>();
			MadaLine line = null;
			while ((line = readNextLine()) != null) {
				if (!line.wordStr.equals(word.word)) {
					if (fixWordMismatch(line, word.word)) {
						tmpLinesFixed.add(line);
					}
				}
				else {
					tmpLinesFixed.add(line);
				}
				tmpLines.add(line);
			}
			
			if (maxLine != -1 && maxLine < tmpLines.size()) {
				ArrayList<MadaLine> tmp = tmpLines;
				tmpLines = new ArrayList<MadaLine>();
				for (int i = 0; i < maxLine; ++i)
					tmpLines.add(tmp.get(i));
			}
			
			if (maxLine != -1 && maxLine < tmpLinesFixed.size()) {
				ArrayList<MadaLine> tmp = tmpLinesFixed;
				tmpLinesFixed = new ArrayList<MadaLine>();
				for (int i = 0; i < maxLine; ++i)
					tmpLinesFixed.add(tmp.get(i));
			}
			
			if (tmpLinesFixed.size() > 0) {
				word.lines = new MadaLine[tmpLinesFixed.size()];
				word.lines = tmpLinesFixed.toArray(word.lines);
			}
			else {
				word.lines = new MadaLine[tmpLines.size()];
				word.lines = tmpLines.toArray(word.lines);
				for (int i = 0; i < word.lines.length; ++i) {
					//System.out.println("Warning: cannot fix " + word.word + " and " + word.lines[i].wordStr);
					hardFixWordMismatch(word.lines[i], word.word);
				}
			}
		}
		
		return word;
	}
	
	public MadaSentence readNextSentence() throws IOException {
		String head = ";;; SENTENCE ";
		String str = reader.readLine();
		while (str != null && !str.startsWith(head)) {
			str = reader.readLine();
		}
		
		if (str == null)
			return null;
		
		MadaSentence sent = new MadaSentence();
		String[] words = str.substring(head.length()).split(" ");
		
		ArrayList<MadaWord> tmpWords = new ArrayList<MadaWord>();
		MadaWord word = null;
		while ((word = readNextWord()) != null) {
			tmpWords.add(word);
		}
		sent.words = new MadaWord[tmpWords.size()];
		sent.words = tmpWords.toArray(sent.words);
		
		Assert(sent.words.length == words.length);
		for (int i = 0; i < sent.words.length; ++i) {
			Assert(words[i].equals(sent.words[i].word));
		}
		return sent;
	}
	
	public static String normalizeAlif(String word) {
		if (word.equals("<") || word.equals(">") || word.equals("{") || word.equals("|"))
			return word;
		
		word = word.replaceAll(">", "A");
		word = word.replaceAll("<", "A");
		word = word.replaceAll("\\{", "A");
		word = word.replaceAll("Y", "y");
		word = word.replaceAll("\\|", "PIPE");
		
		return word;
	}
	
	public static String normalizeForm(String form, String pos) {
		if (pos.equals("FOREIGN") || pos.equals("LATIN")) {
			if (form.replaceAll("[aeiouKFN`~_-]", "").isEmpty())
				return form;
		}
		if (form.matches("[`_-]+"))
			return form;
		else if (form.equals("F") && pos.indexOf("CASE") == -1)
			return form;
		else if (form.equals("-PLUS-"))
			return "PLUS";
		else if (form.matches("[0-9-]+-")) {
			return form.replaceAll("-", "");
		}
		
		if (form.length() > 1 && form.charAt(form.length() - 1) == '+') {
			form = form.substring(0, form.length() - 1);
		}
		
		form = form.replaceAll("-LRB-", "(");
		form = form.replaceAll("-RRB-", ")");
		form = form.replaceAll("\\(null\\)", "");
		form = form.replaceAll("\\[null\\]", "");
		form = form.replaceAll("\\(nll\\)", "");
		form = form.replaceAll("\\[nll\\]", "");
		form = form.replaceAll("<COMMA>", ",");
		String ret = form.replaceAll("[aeiouKFN`~_-]", "");
		return ret;
	}
	
	public String normalizePos(String pos) {
		if (pos.startsWith("CASE")) {
			pos = "CASE";
		}
		else {
			Assert(fine2coarse.containsKey(pos));
			pos = fine2coarse.get(pos);
		}
		
		return pos;
	}
	
	public String convertToCore12(String pos) {
		if (!coarse2core12.containsKey(pos))
			System.out.println(pos);
		Assert(coarse2core12.containsKey(pos));
		return coarse2core12.get(pos);
	}

	public static double convertScore(double s) {
		return Math.max(0, Math.exp(s) - 1);
	}
	
	public SegStruct buildSegStruct(MadaWord word) {
		SegStruct segStruct = new SegStruct();
		segStruct.word = word.word;
		
		for (int i = 0; i < word.lines.length; ++i) {
			MadaLine line = word.lines[i];
			String segStr = line.segStr;
			double prob = convertScore(line.score / (i + 1));

			int index = 0;
			for (; index < segStruct.segStr.size(); ++index) {
				if (segStruct.segStr.get(index).equals(segStr))
					break;
			}
			
			if (index == segStruct.segStr.size()) {
				// new seg
				segStruct.segStr.add(segStr);
				segStruct.segProb.add(prob);
				
				SegPosStruct posStruct = new SegPosStruct();
				for (int j = 0; j < line.seg.length; ++j) {
					posStruct.seg.add(line.seg[j]);
					if (j == line.morphIndex) {
						posStruct.lemma.add(line.lemma);
					}
					else {
						posStruct.lemma.add(line.seg[j]);
					}
					HashMap<String, Double> posCand = new HashMap<String, Double>();
					posCand.put(line.pos[j], prob);
					posStruct.segPosDist.add(posCand);
				}
				
				posStruct.morphIndex = line.morphIndex;
				posStruct.detIndex = line.detIndex;
				posStruct.morphInfo = line.per + "/" + line.gen + "/" + line.num;
				
				segStruct.segData.add(posStruct);
			}
			else {
				// old seg
				Assert(prob < segStruct.segProb.get(index));
				SegPosStruct posStruct = segStruct.segData.get(index);
				
				for (int j = 0; j < line.seg.length; ++j) {
					Assert(line.seg[j].equals(posStruct.seg.get(j)));
					
					HashMap<String, Double> posCand = posStruct.segPosDist.get(j);
					if (posCand.containsKey(line.pos[j])) {
						Assert(prob < posCand.get(line.pos[j]));
					}
					else {
						posCand.put(line.pos[j], prob);
					}
				}
			}

		}
		
		segStruct.normalize();
		segStruct.sort();
		
		return segStruct;
	}
	
	public SegStruct[] generateCandidate(MadaSentence sent) {
		SegStruct[] list = new SegStruct[sent.words.length];
		for (int i = 0; i < sent.words.length; ++i) {
			list[i] = buildSegStruct(sent.words[i]);
		}		
		return list;
	}
	
	public static void Assert(boolean assertion) 
	{
		if (!assertion) {
			(new Exception()).printStackTrace();
			System.exit(1);
		}
	}
}
