import java.io.*;
import java.util.*;

class SpmrlSeg {
	String seg;
	String lemma;
	String pos;
	boolean det;
	String per;
	String gen;
	String num;
	boolean hasMorph;
	int tmpHead;
	HeadIndex head;
	String depLabel;
	int st;
	int en;
}

class SpmrlWord {
	String word;
	String segStr;
	SpmrlSeg[] segs;
}

class SpmrlSentence {
	SpmrlWord[] words;
}

class SegPosStruct {
	ArrayList<String> seg;		// string of each seg
	ArrayList<String> lemma;	// lemma of each seg
	ArrayList<Boolean> Aldet;
	ArrayList<HashMap<String, Double>> segPosDist;		// POS distribution for each seg;
	
	int morphIndex;
	int detIndex;
	String morphInfo;
	
	public SegPosStruct() {
		seg = new ArrayList<String>();
		lemma = new ArrayList<String>();
		Aldet = new ArrayList<Boolean>();
		segPosDist = new ArrayList<HashMap<String, Double>>();
		morphIndex = -1;
		detIndex = -1;
		morphInfo = "na/na/na";
	}
	
	public void dump(BufferedWriter bw) throws IOException {
		for (int i = 0; i < seg.size(); ++i) {
			if (i > 0)
				bw.write("&&");
			bw.write(seg.get(i) + "@#" + lemma.get(i));
			HashMap<String, Double> posDist = segPosDist.get(i);
			String[] posList = new String[posDist.size()];
			double[] probList = new double[posDist.size()];
			int pp = 0;
			for (String s : posDist.keySet()) {
				posList[pp] = s;
				probList[pp] = posDist.get(s);
				++pp;
			}
			for (int p1 = 0; p1 < posList.length; ++p1)
				for (int p2 = p1 + 1; p2 < posList.length; ++p2)
					if (probList[p1] < probList[p2]) {
						double tmpd = probList[p1];
						probList[p1] = probList[p2];
						probList[p2] = tmpd;
						String tmps = posList[p1];
						posList[p1] = posList[p2];
						posList[p2] = tmps;
					}
			for (int l = 0; l < posList.length; ++l) {
				bw.write("@#" + posList[l] + "_" + String.format("%.5f", probList[l]));
			}
		}
		bw.write("||" + detIndex + "||" + morphIndex + "||" + morphInfo);
	}
}

class SegStruct {
	String word;
	ArrayList<String> segStr;
	ArrayList<Double> segProb;
	ArrayList<SegPosStruct> segData;
	
	public SegStruct() {
		segStr = new ArrayList<String>();
		segProb = new ArrayList<Double>();
		segData = new ArrayList<SegPosStruct>();
	}
	
	public SegStruct(String word) {
		this.word = word;
		segStr = new ArrayList<String>();
		segProb = new ArrayList<Double>();
		segData = new ArrayList<SegPosStruct>();
	}
	
	public void normalize() {
		double sumProb = 0.0;
		for (int i = 0; i < segProb.size(); ++i) {
			sumProb += segProb.get(i);
		}
		if (sumProb < 1e-4) {
			sumProb = 1.0;
			for (int i = 0; i < segProb.size(); ++i) {
				segProb.set(i, 1.0 / segProb.size());
			}
		}
		for (int i = 0; i < segProb.size(); ++i) {
			segProb.set(i, segProb.get(i) / sumProb);
			
			SegPosStruct posStruct = segData.get(i);
			for (int j = 0; j < posStruct.segPosDist.size(); ++j) {
				HashMap<String, Double> posDist = posStruct.segPosDist.get(j);
				double sumPosProb = 0.0;
				for (String s : posDist.keySet())
					sumPosProb += posDist.get(s);
				if (sumPosProb < 1e-4) {
					sumPosProb = 1.0;
					for (String s : posDist.keySet())
						posDist.put(s, 1.0 / sumPosProb);
				}
				for (String s : posDist.keySet())
					posDist.put(s, posDist.get(s) / sumPosProb);
			}
		}
		
		// remove zero prob candidate
		for (int i = 0; i < segProb.size(); ++i) {
			if (segProb.get(i) < 1e-4) {
				segProb.remove(i);
				segStr.remove(i);
				segData.remove(i);
				i--;
			}
		}
		
		if (segProb.size() == 0) {
			System.out.println("normalize bug 1");
		}
	}
	
	public void dump(BufferedWriter bw) throws IOException {
		bw.write(word);
		for (int i = 0; i < segStr.size(); ++i) {
			bw.write("\t");
			segData.get(i).dump(bw);
			bw.write("||" + String.format("%.5f", segProb.get(i)));
		}
		bw.newLine();
	}
	
	public void sort() {
		for (int i = 0; i < segStr.size(); ++i) {
			for (int j = i + 1; j < segStr.size(); ++j) {
				if (segProb.get(i) < segProb.get(j)) {
					double tmpd = segProb.get(i);
					segProb.set(i, segProb.get(j));
					segProb.set(j, tmpd);
					
					String tmps = segStr.get(i);
					segStr.set(i, segStr.get(j));
					segStr.set(j, tmps);
					
					SegPosStruct tmpp = segData.get(i);
					segData.set(i, segData.get(j));
					segData.set(j, tmpp);
				}
			}
		}
	}
}

class HeadIndex {
	public int hWord;
	public int hSeg;
	
	public HeadIndex() {
		hWord = -1;
		hSeg = 0;
	}
	
	public HeadIndex(int hWord, int hSeg) {
		this.hWord = hWord;
		this.hSeg = hSeg;
	}

	public String toString() {
		return new Integer(hWord).toString() + "/" + new Integer(hSeg).toString();
	}
}

class WordInstance {
	ArrayList<HeadIndex> dep;
	ArrayList<Integer> depid;	// convert to head index later
	HashSet<Integer> id;		// the id of the node
	ArrayList<String> form;
	ArrayList<String> pos;
	ArrayList<Boolean> Aldet;
	ArrayList<String> lab;
	ArrayList<Integer> part;	//0: prefix; 1: root; 2: suffix
	
	String wordStr;
	String posStr;
	String segStr;
	
	public WordInstance() {
		dep = new ArrayList<HeadIndex>();
		depid = new ArrayList<Integer>();
		id = new HashSet<Integer>();
		form = new ArrayList<String>();
		pos = new ArrayList<String>();
		lab = new ArrayList<String>();
		part = new ArrayList<Integer>();
		Aldet = new ArrayList<Boolean>();
	}
}

class SentInstance {
	ArrayList<WordInstance> word;
	
	public SentInstance() {
		word = new ArrayList<WordInstance>();
	}
}

public class SpmrlReader {
	
	public BufferedReader reader;
	public HashMap<String, String> core12Map = new HashMap<String, String>();
	
	private boolean isTest;
	
	public int numWord;
	public int corrSegment;
	public int oracleCorrSegment;
	public int numSegment;
	public int corrPos;
	public int oracleCorrPos;
	
	public int goldSeg = 0;
	public int predSeg = 0;
	public double corrSeg = 0.0;
	public double corrP = 0.0;
	
	public HashMap<String, SegStruct> goldSegDict = null;
	public boolean useGoldSegDict = true;
	
	public SpmrlReader() throws IOException {
		loadCore12Map();		
		numWord = 0;
		corrSegment = 0;
		oracleCorrSegment = 0;
		numSegment = 0;
		corrPos = 0;
		oracleCorrPos = 0;
	}
	
	public void open(String file) throws IOException {
		reader = new BufferedReader(new FileReader(file));
		isTest = file.indexOf("test") != -1;
	}
	
	public void close() throws IOException {
		if (reader != null)
			reader.close();
	}

	public void loadCore12Map() throws IOException {
		core12Map = new HashMap<String, String>();
		
		BufferedReader br = new BufferedReader(new FileReader("../data/spmrl/core12map.txt"));
		String str = null;
		while ((str = br.readLine()) != null) {
			String[] data = str.split("\\s+");
			core12Map.put(data[0], data[1]);
		}
		br.close();
	}
	
	public void buildGoldSegDict() throws IOException {
		System.out.print("Build gold segment dictionary...");
		String fileName = "train";
		
		SpmrlReader reader = new SpmrlReader();
		reader.open("../data/spmrl/" + fileName + ".Arabic");

		goldSegDict = new HashMap<String, SegStruct>();
		SpmrlSentence sent = null;
		while ((sent = reader.readNextSentence()) != null) {
			for (int i = 0; i < sent.words.length; ++i) {
				SpmrlWord word = sent.words[i];
				if (!goldSegDict.containsKey(word.word)) {
					goldSegDict.put(word.word, new SegStruct());
				}
				
				SegStruct seg = goldSegDict.get(word.word);
				seg.word = word.word;
				int index = 0;
				for (; index < seg.segStr.size(); ++index)
					if (seg.segStr.get(index).equals(word.segStr))
						break;
				
				double prob = 0.01;
				
				if (index == seg.segStr.size()) {
					// new
					seg.segStr.add(word.segStr);
					seg.segProb.add(prob);
					
					SegPosStruct posStruct = new SegPosStruct();
					for (int j = 0; j < word.segs.length; ++j) {
						posStruct.seg.add(word.segs[j].seg);
						posStruct.lemma.add(word.segs[j].lemma);
						HashMap<String, Double> posCand = new HashMap<String, Double>();
						posCand.put(word.segs[j].pos, prob);
						posStruct.segPosDist.add(posCand);
						
						boolean hasMorphValue = false;
						if (!word.segs[j].gen.equals("na"))
							hasMorphValue = true;
						if (!word.segs[j].per.equals("na"))
							hasMorphValue = true;
						if (!word.segs[j].num.equals("na"))
							hasMorphValue = true;
						
						if (hasMorphValue && posStruct.morphIndex == -1) {
							posStruct.morphIndex = j;
							posStruct.morphInfo = word.segs[j].per + "/" + word.segs[j].gen + "/" + word.segs[j].num;
						}
						
						if (word.segs[j].det && posStruct.detIndex == -1) {
							posStruct.detIndex = j;
						}
					}
					seg.segData.add(posStruct);
				}
				else {
					// old
					seg.segProb.set(index, seg.segProb.get(index) + prob);
					SegPosStruct posStruct = seg.segData.get(index);
					
					for (int j = 0; j < word.segs.length; ++j) {
						MadaReader.Assert(word.segs[j].seg.equals(posStruct.seg.get(j)));
						
						HashMap<String, Double> posCand = posStruct.segPosDist.get(j);
						if (!posCand.containsKey(word.segs[j].pos)) {
							posCand.put(word.segs[j].pos, prob);
						}
						else {
							posCand.put(word.segs[j].pos, posCand.get(word.segs[j].pos) + prob);
						}
					}
				}
			}
		}
		
		reader.close();
		System.out.println("Done.");
	}
	
	
	public boolean splitFromNext(String str) {
		return str.indexOf("split_from_next=+") != -1;
	}

	public boolean splitFromPrevious(String str) {
		return str.indexOf("split_from_previous=+") != -1;
	}
	
	public String map2Core12(String pos) {
		MadaReader.Assert(core12Map.containsKey(pos));
		return core12Map.get(pos);
	}
	
	public void fixMorphInfo(SpmrlWord word) {
		int[] p = getPosPriority(word);
		int highestPriority = -1;
		int highestPriorityIndex = -1;
		for (int i = 0; i < word.segs.length; ++i)
			if (p[i] > highestPriority) {
				highestPriority = p[i];
				highestPriorityIndex = i;
			}
		//System.out.print("Keep: " + word.segs[highestPriorityIndex].pos + "; Ignore: ");
		for (int i = 0; i < word.segs.length; ++i) {
			if (i == highestPriorityIndex)
				continue;
			SpmrlSeg seg = word.segs[i];
			if (!seg.per.equals("na") || !seg.gen.equals("na") || !seg.num.equals("na")) {
				//System.out.print(seg.pos + " ");
				seg.per = "na";
				seg.gen = "na";
				seg.num = "na";
				seg.hasMorph = false;
			}
		}
		//System.out.println();
	}

	public String normalizeForm(String form) {
		form = form.replaceAll("-LRB-", "(");
		form = form.replaceAll("-RRB-", ")");
		
		if (!form.equals("+")) {
			form = form.replaceAll("\\+", "");
		}
		
		return form;
	}
	
	public int[] getPosPriority(SpmrlWord word) {
		int[] p = new int[word.segs.length];
		
		for (int i = 0; i < word.segs.length; ++i) {
			if (word.segs[i].pos.equals("N")) {
				p[i] = 4;
			}
			else if (word.segs[i].pos.equals("PN")
					|| word.segs[i].pos.equals("ABBREV")
					|| word.segs[i].pos.equals("AJ")) {
				p[i] = 3;
			}
			else if (word.segs[i].pos.equals("V")) {
				p[i] = 2;
			}
			else if (word.segs[i].pos.equals("PRO")
						|| word.segs[i].pos.equals("AV")
						|| word.segs[i].pos.equals("REL")) {
				p[i] = 1;
			}
			else {
				p[i] = 0;
			}
		}
		
		return p;
	}
	
	public boolean checkPosPriority(SpmrlWord word) {
		int priority = -1;
		int[] p = getPosPriority(word);
		for (int i = 0; i < word.segs.length; ++i) {
			if (word.segs[i].hasMorph) {
				priority = p[i];
			}
		}
		if (priority == 0)
			return false;
		
		if (priority > 0) {
			// has morphology
			for (int i = 0; i < word.segs.length; ++i) {
				if (!word.segs[i].hasMorph) {
					if (p[i] >= priority)
						return false;
				}
			}
		}
		return true;
	}
	
	public void fixGoldError(SpmrlSeg seg) {
		if (seg.seg.equals("b") && seg.pos.equals("N"))
			seg.pos = "P";
		else if (seg.seg.equals("h") && seg.pos.equals("N"))
			seg.pos = "PRO";
		else if (seg.seg.equals("y") && seg.pos.equals("N"))
			seg.pos = "PRO";
		else if (seg.seg.equals("w") && seg.pos.equals("N"))
			seg.pos = "CONJ";
	}
	
	public void checkAldet(SpmrlWord word) {
		int hasDetNum = 0;
		for (int i = 0; i < word.segs.length; ++i) {
			if (word.segs[i].det) {
				String pos = word.segs[i].pos;
				MadaReader.Assert(pos.equals("N") || pos.equals("AJ") || pos.equals("ABBREV") || pos.equals("PN"));
				hasDetNum++;
			}
		}
		MadaReader.Assert(hasDetNum <= 1);
	}

	public SpmrlWord readNextWord() throws IOException {
		SpmrlWord word = new SpmrlWord();
		
		String str = reader.readLine();
		if (str == null || str.isEmpty())
			return null;
		
		ArrayList<String> strList = new ArrayList<String>();
		strList.add(str);
		while (splitFromNext(str)) {
			str = reader.readLine();
			MadaReader.Assert(str != null && splitFromPrevious(str));
			strList.add(str);
		}
		
		int length = strList.size();
		word.segs = new SpmrlSeg[length];
		
		int hasMorphInfo = 0;
		word.word = "";
		for (int i = 0; i < strList.size(); ++i) {
			String[] data = strList.get(i).split("\t");
			word.segs[i] = new SpmrlSeg();
			SpmrlSeg seg = word.segs[i];
			//seg.seg = normalizeForm(data[1]);
			//seg.lemma = normalizeForm(data[2]);
			
			String[] morphData = data[5].split("\\|");
			boolean hasMorph = false;
			for (int j = 0; j < morphData.length; ++j) {
				if (morphData[j].startsWith("core12")) {
					seg.pos = map2Core12(morphData[j].split("=")[1]);
				}
				else if (morphData[j].startsWith("prc0")) {
					seg.det = morphData[j].split("=")[1].equals("Al_det");
				}
				else if (morphData[j].startsWith("per")) {
					seg.per = morphData[j].split("=")[1];
					hasMorph |= !seg.per.equals("na");
				}
				else if (morphData[j].startsWith("gen")) {
					seg.gen = morphData[j].split("=")[1];
					hasMorph |= !seg.gen.equals("na");
				}
				else if (morphData[j].startsWith("num")) {
					seg.num = morphData[j].split("=")[1];
					hasMorph |= !seg.num.equals("na");
				}
			}
			if (seg.pos == null)
				seg.pos = "N";
			
			seg.seg = MadaReader.normalizeForm(data[1], seg.pos);
			seg.lemma = MadaReader.normalizeForm(data[2], seg.pos);
			MadaReader.Assert(!seg.seg.isEmpty());
			if (seg.lemma.isEmpty())
				seg.lemma = seg.seg;
			word.word += seg.seg;
			
			seg.hasMorph = hasMorph;
			
			hasMorphInfo += hasMorph ? 1 : 0;
			
			if (!isTest)
				fixGoldError(seg);
			
			seg.tmpHead = Integer.parseInt(data[6]);
			seg.depLabel = data[7];
		}

		// only one seg has morph info
		if (hasMorphInfo > 1 || !checkPosPriority(word)) {
			fixMorphInfo(word);
		}
		
		if (!checkPosPriority(word)) {
			for (int i = 0; i < strList.size(); ++i)
				System.out.println(strList.get(i));
		}
		MadaReader.Assert(checkPosPriority(word));
		
		checkAldet(word);
		
		word.segStr = "";
		for (int i = 0; i < word.segs.length; ++i) {
			if (i > 0) word.segStr+= "+";
			word.segStr += word.segs[i].seg;
		}

		return word;
	}
	
	public SpmrlSentence readNextSentence() throws IOException {
		ArrayList<SpmrlWord> tmpWord = new ArrayList<SpmrlWord>();
		SpmrlWord word = null;
		while ((word = readNextWord()) != null) {
			tmpWord.add(word);
		}
		if (tmpWord.size() == 0)
			return null;
		
		SpmrlSentence sent = new SpmrlSentence();
		sent.words = new SpmrlWord[tmpWord.size()];
		sent.words = tmpWord.toArray(sent.words);
		
		buildHead(sent);
		
		return sent;
	}
	
	public void buildHead(SpmrlSentence sent) {
		HashMap<Integer, HeadIndex> map = new HashMap<Integer, HeadIndex>();
		map.put(0, new HeadIndex(0, 0));
		int cnt = 0;
		for (int i = 0; i < sent.words.length; ++i) {
			SpmrlWord word = sent.words[i];
			for (int j = 0; j < word.segs.length; ++j) {
				cnt++;
				map.put(cnt, new HeadIndex(i + 1, j));
			}
		}
		
		for (int i = 0; i < sent.words.length; ++i) {
			SpmrlWord word = sent.words[i];
			for (int j = 0; j < word.segs.length; ++j) {
				word.segs[j].head = map.get(word.segs[j].tmpHead);
			}
		}
	}
	
	public void outputGoldSentence(SpmrlSentence sent, BufferedWriter bw) throws IOException {
		for (int i = 0; i < sent.words.length; ++i) {
			SpmrlWord word = sent.words[i];
			for (int j = 0; j < word.segs.length; ++j) {
				bw.write("" + (i + 1) + "/" + j);
				
				SpmrlSeg seg = word.segs[j];
				bw.write("\t" + seg.seg + "\t" + seg.lemma + "\t" + seg.pos + "\t" + seg.pos);
				bw.write("\t" + "det=" + (seg.det ? "y" : "n") + "|per=" + seg.per + "|gen=" + seg.gen + "|num=" + seg.num);
				bw.write("\t" + seg.head.toString() + "\t" + seg.depLabel + "\t_\t_\n");
			}
		}
		bw.newLine();
	}

	public void evaluateMadaPredict(SegStruct[] candList, SpmrlSentence gold) {
		MadaReader.Assert(candList.length == gold.words.length);
		
		numWord += candList.length;
		for (int i = 0; i < candList.length; ++i) {
			SpmrlWord word = gold.words[i];
			SegStruct cand = candList[i];
			
			String goldSegStr = word.segStr;
			
			SegPosStruct corrSegStr = null;
			if (cand.segStr.get(0).equals(goldSegStr)) {
				corrSegment++;
				corrSegStr = cand.segData.get(0);
			}
			
			numSegment += word.segs.length;
			
			if (corrSegStr != null) {
				MadaReader.Assert(word.segs.length == corrSegStr.seg.size());
				for (int j = 0; j < word.segs.length; ++j) {
					HashMap<String, Double> posDist = corrSegStr.segPosDist.get(j);
					if (posDist.containsKey(word.segs[j].pos)) {
						// check if pos is also the first
						double prob = posDist.get(word.segs[j].pos);
						boolean posIsFirst = true;
						for (String s : posDist.keySet()) {
							if (posDist.get(s) > prob + 1e-6) {
								posIsFirst = false;
								break;
							}
						}
						if (posIsFirst) {
							corrPos++;
						}
					}
				}
			}

			SegPosStruct pred = cand.segData.get(0);
			goldSeg += word.segs.length;
			predSeg += pred.seg.size();
			HashMap<String, String> predPosMap = new HashMap<String, String>();
			int st = 0;
			for (int j = 0; j < pred.seg.size(); ++j) {
				int en = st + pred.seg.get(j).length();
				HashMap<String, Double> posDist = pred.segPosDist.get(j);
				double maxProb = Double.NEGATIVE_INFINITY;
				String maxPos = "";
				for (String s : posDist.keySet()) {
					if (posDist.get(s) > maxProb) {
						maxProb = posDist.get(s);
						maxPos = s;
					}
				}
				predPosMap.put("" + st + "," + en, maxPos);
				st = en;
			}
			
			st = 0;
			for (int j = 0; j < word.segs.length; ++j) {
				int en = st + word.segs[j].seg.length();
				String str = "" + st + "," + en;
				if (predPosMap.containsKey(str)) {
					corrSeg++;
					if (predPosMap.get(str).equals(word.segs[j].pos)) {
						corrP++;
					}
				}
				st = en;
			}
		}
	}
	
	public void evaluateOracle(SegStruct[] candList, SpmrlSentence gold) {
		MadaReader.Assert(candList.length == gold.words.length);
		
		for (int i = 0; i < candList.length; ++i) {
			SpmrlWord word = gold.words[i];
			SegStruct cand = candList[i];
			
			String goldSegStr = word.segStr;
			
			SegPosStruct corrSeg = null;
			for (int j = 0; j < cand.segStr.size(); ++j) {
				if (cand.segStr.get(j).equals(goldSegStr)) {
					oracleCorrSegment++;
					corrSeg = cand.segData.get(j);
				}
			}
			
			if (corrSeg != null) {
				MadaReader.Assert(word.segs.length == corrSeg.seg.size());
				for (int j = 0; j < word.segs.length; ++j) {
					HashMap<String, Double> posDist = corrSeg.segPosDist.get(j);
					if (posDist.containsKey(word.segs[j].pos)) {
						oracleCorrPos++;
					}
				}
			}
		}
	}
	
	public void addSegDict(SegStruct s1, SegStruct s2) {

		for (int z = 0; z < s2.segStr.size(); ++z) {
			String segStr = s2.segStr.get(z);
			double prob = s2.segProb.get(z);
			SegPosStruct ps = s2.segData.get(z);
			int index = 0;
			for (; index < s1.segStr.size(); ++index)
				if (s1.segStr.get(index).equals(segStr))
					break;
			
			if (index == s1.segStr.size()) {
				// new
				s1.segStr.add(segStr);
				s1.segProb.add(prob);

				SegPosStruct posStruct = new SegPosStruct();
				for (int i = 0; i < ps.seg.size(); i++) {
					posStruct.seg.add(ps.seg.get(i));
					posStruct.lemma.add(ps.lemma.get(i));
					HashMap<String, Double> posCand = new HashMap<String, Double>();
					posCand.putAll(ps.segPosDist.get(i));
					posStruct.segPosDist.add(posCand);
				}
				posStruct.detIndex = ps.detIndex;
				posStruct.morphIndex = ps.morphIndex;
				posStruct.morphInfo = ps.morphInfo;
				
				s1.segData.add(posStruct);
			}
			else {
				// old
				s1.segProb.set(index, Math.max(s1.segProb.get(index), prob));
				SegPosStruct posStruct = s1.segData.get(index);

				posStruct.detIndex = ps.detIndex;
				posStruct.morphIndex = ps.morphIndex;
				posStruct.morphInfo = ps.morphInfo;

				if (ps.seg.size() != posStruct.seg.size())
					System.out.println("bug add gold 2");

				for (int i = 0; i < ps.seg.size(); ++i) {
					if (!ps.seg.get(i).equals(posStruct.seg.get(i))) {
						System.out.println("bug add gold 3: " + ps.seg.get(i) + "\t" + posStruct.seg.get(i));
					}

					HashMap<String, Double> posCand = posStruct.segPosDist.get(i);
					for (String pos : ps.segPosDist.get(i).keySet()) {
						double posProb = ps.segPosDist.get(i).get(pos);
						if (posCand.containsKey(pos)) {
							posCand.put(pos, Math.max(posCand.get(pos), posProb));
						}
						else {
							posCand.put(pos, posProb);
						}
					}
				}
			}
		}
	}
	
	public void addGoldSegDict(SegStruct[] list) {
		for (int i = 0; i < list.length; ++i) {
			if (goldSegDict.containsKey(list[i].word)) {
				addSegDict(list[i], goldSegDict.get(list[i].word));
//				SegStruct seg = goldSegDict.get(list[i].word);
//				if (list[i].word.equals("l<TlAq")) {
//					System.out.println("cccc");
//					for (int y = 0; y < seg.segData.size(); ++y)
//						System.out.println(seg.segData.get(y).morphInfo);
//					System.out.println();
//				}
			}
			list[i].normalize();
			list[i].sort();
		}
	}
	
	public void outputCandidate(SegStruct[] list, BufferedWriter bw) throws IOException{
		for (int i = 0; i < list.length; ++i) {
			list[i].dump(bw);
		}
		bw.newLine();
	}

}
