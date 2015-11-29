import java.io.*;
import java.util.*;

public class SpmrlDataGenerator {

	public static boolean sameSentence(SpmrlSentence s1, MadaSentence s2) {
		String x1 = "";
		for (int i = 0; i < s1.words.length; ++i)
			x1 += s1.words[i].word + " ";
		
		String x2 = "";
		for (int i = 0; i < s2.words.length; ++i) 
			x2 += s2.words[i].word + " ";
		
		if (!x1.equals(x2)) {
			System.out.println(x1);
			System.out.println(x2);	
		}
		return x1.equals(x2);
	}
	
	public static void generateFile(int split, String fileName) throws IOException {
		int maxLength = 70;
		
		SpmrlReader reader = new SpmrlReader();
		reader.open("../data/" + fileName + ".Arabic");
		if (reader.useGoldSegDict)
			reader.buildGoldSegDict();
		
		MadaReader[] madaReader = new MadaReader[split];
		int maxLine = -1;
		for (int i = 0; i < split; ++i) {
			madaReader[i] = new MadaReader();
			madaReader[i].open("../data/" + fileName + i + ".mada");
			madaReader[i].maxLine = maxLine;
		}
		
		BufferedWriter bw = new BufferedWriter(new FileWriter("../data/spmrl/spmrl.seg.full." + fileName));
		
		SpmrlSentence sent = null;
		int id = 0;
		int cnt = 0;
		while ((sent = reader.readNextSentence()) != null) {
			MadaSentence madaSent = madaReader[id].readNextSentence();

			MadaReader.Assert(sameSentence(sent, madaSent));
			
			int segNum = 0;
			for (int i = 0; i < sent.words.length; ++i)
				segNum += sent.words[i].segs.length;
			if (segNum <= maxLength) {
				reader.outputGoldSentence(sent, bw);
				SegStruct[] segStruct =  madaReader[id].generateCandidate(madaSent);
				for (int z = 0; z < segStruct.length; ++z) {
					if (segStruct[z].word.equals("l<TlAq")) {
						System.out.println("aaaa");
						for (int y = 0; y < segStruct[z].segData.size(); ++y)
							System.out.println(segStruct[z].segData.get(y).morphInfo);
						System.out.println();
					}
				}
				reader.evaluateMadaPredict(segStruct, sent);
				if (maxLine == -1 || maxLine >= 5) {
					reader.addGoldSegDict(segStruct);
//					for (int z = 0; z < segStruct.length; ++z) {
//						if (segStruct[z].word.equals("l<TlAq")) {
//							System.out.println("bbbb");
//							for (int y = 0; y < segStruct[z].segData.size(); ++y)
//								System.out.println(segStruct[z].segData.get(y).morphInfo);
//							System.out.println();
//						}
//					}
				}
				reader.outputCandidate(segStruct, bw);
				reader.evaluateOracle(segStruct, sent);
			}
			
			id = (id + 1) % split;
			cnt++;
		}
		
		reader.close();
		for (int i = 0; i < split; ++i)
			madaReader[i].close();
		
		bw.close();
		
		System.out.println("sentence: " + cnt);
		System.out.println("Total word: " + reader.numWord + " Correct seg: " + reader.corrSegment + " Oracle: " + reader.oracleCorrSegment);
		System.out.println("Total segment: " + reader.numSegment + " Correct pos: " + reader.corrPos + " Oracle: " + reader.oracleCorrPos);

		
		double segPre = reader.corrSeg / reader.predSeg;
		double segRec = reader.corrSeg / reader.goldSeg;
		System.out.println("Seg pre/rec/f1: " + segPre + " " + segRec + " " + (2 * segPre * segRec) / (segPre + segRec));
		double posPre = reader.corrP/ reader.predSeg;
		double posRec = reader.corrP / reader.goldSeg;
		System.out.println("pos pre/rec/f1: " + posPre + " " + posRec + " " + (2 * posPre * posRec) / (posPre + posRec));
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try {
			generateFile(10, "train");
			generateFile(1, "dev");
			generateFile(1, "test");
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

}
