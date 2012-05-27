#include "pitchdetector.h"
#include "image.h"
#include "landmarkreader.h"
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream> //TODO: debug

using namespace cv;

PitchDetector::PitchDetector(QString positionFile) : positionFile(positionFile){
	positions = deserialize();
}

PitchDetector& PitchDetector::retainDir(QString retainDir){
	auto keys = positions.keys();
	for(int i=0; i<keys.count(); ++i){
		if(keys.at(i).contains(retainDir)){
			positions.remove(keys.at(i));
		}
	}
	return *this;
}

bool PitchDetector::containsTies(QVector<long> vec){
	int max = 0;

	foreach(long v, vec){
		if(v>max){
			max = v;
		}
	}

	return (vec.count(max) > 1) ? true : false;
}

double PitchDetector::positionFromFile(QString filename){
	QFileInfo info1(filename);
	auto allPositions = deserialize();
	auto keys = allPositions.keys();
	for(int i=0; i<keys.count(); ++i){
		QFileInfo info2(keys.at(i));
		if(info1.fileName() == info2.fileName()){
			return allPositions[keys.at(i)].second;
		}
	}
	//TODO: moet dit een error smijten
}

QVector<long> PitchDetector::detectPitch(QString filename, double fuzziness){
	QMap<long, QVector<float> > result;

	auto ks = positions.keys();
	for(int i=0; i<ks.count(); ++i){
		auto pair = positions.value(ks.at(i));
		result[pair.first].push_back(pair.second);
	}

	double diff = positionFromFile(filename);

	auto keys = result.keys();
	QVector<long> support(keys.count(), 0);
	for(int i=0; i<keys.count(); ++i){
		auto values = result.value(keys.at(i));
		for(int j=0; j<values.count(); ++j){
			if(isnan(values.at(j))){
				values.replace(j, 0); //TODO: temporary workaround
			}
			if(isnan(diff)){
				diff = 0; //TODO: temporary workaround
			}

			double a = std::abs(values.at(j) - diff);
			if(a < fuzziness){
				support.replace(i, support.at(i)+1);
			}
		}
	}

	return support;
}

long PitchDetector::operator()(QString filename, double fuzziness){
	auto support = detectPitch(filename, fuzziness);
	int counter = 0;
	while(containsTies(support) && fuzziness > 0){
//		std::cout << "Tie" << std::endl; //TODO: dit mag later weg
		++counter;
		fuzziness -= 0.0002; //0.0002
		support = detectPitch(filename, fuzziness);
	}
	while(containsTies(support)){
//		std::cout << "Tie" << std::endl; //TODO: dit mag later weg
		++counter;
		fuzziness += 0.002;
		support = detectPitch(filename, fuzziness);
	}

	int maxSupport = 0;
	int index = 0;
	QVector<long> keys;
	keys << -2 << -1 << 0 << 1 << 2;
	for(int i=0; i<keys.count(); ++i){
		if(support.at(i) > maxSupport){
			maxSupport = support.at(i);
			index = i;
		}
	}

	//TODO: return the pitch
	return keys.at(index);
}

long PitchDetector::pitch(QString filename){
	filename = filename.split("/").last(); //TODO: kan dit niet mooier met fileInfo?
	if(filename.contains("PR")){
		int index = filename.lastIndexOf("PR")+3;
		filename = filename.mid(index, filename.length()-index-6);
		if(filename == "D"){
			return -2;
		} else if(filename == "SD"){
			return -1;
		} else if(filename == "SU"){
			return 1;
		} else if (filename == "U"){
			return 2;
		}
	}

	return 0;
}

QMap<QString, QPair<long, double> > PitchDetector::deserialize(){
	QMap<QString, QPair<long, double> > result;

	QFile file(positionFile);
	if(file.open(QIODevice::ReadOnly)){
		QTextStream stream(&file);

		while(!stream.atEnd()){
			QString key;
			QPair<long, double> value;
			key = stream.readLine();
			value.first = stream.readLine().toLong();
			value.second = stream.readLine().toDouble();

			result.insert(key, value);
		}
	}

	file.close();
	return result;
}
