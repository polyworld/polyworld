#!/usr/bin/env python

import datalib

agentInfos = {}
epochs = {}
epochLen = 1000
run = './run'

class AgentInfo:
	def __init__( self ):
		self.foodSeen = set()
		self.goodSeen = False
		self.badSeen = False

class Epoch:
	def __init__( self, timestep ):
		self.timestep = timestep
		self.finalTimestep = timestep
		self.goodAllCount = 0
		self.badAllCount = 0
		self.goodAllEnergy = 0.0
		self.badAllEnergy = 0.0
		self.goodIgnoreFirstCount = 0
		self.badIgnoreFirstCount = 0
		self.goodSecondPieceCount = 0
		self.badSecondPieceCount = 0
		self.goodUniqueCount = 0
		self.badUniqueCount = 0
		self.goodAfterBad = 0
		self.goodAfterGood = 0
		self.afterBadCount = 0
		self.afterGoodCount = 0

	def addEvent( self, eventRow ):
		if eventRow['EventType'] != 'E':
			return

		self.finalTimestep = max( self.finalTimestep, eventRow['T'] )

		energy = eventRow['Energy0']
		isgood = energy >= 0
		if isgood:
			self.goodAllCount += 1
			self.goodAllEnergy += energy
		else:
			self.badAllCount += 1
			self.badAllEnergy += energy

		agent = eventRow['Agent']
		food = eventRow['ObjectNumber']

		try:
			agentInfo = agentInfos[agent]
			firstEvent = False
		except:
			firstEvent = True
			agentInfo = AgentInfo()
			agentInfos[agent] = agentInfo

		if agentInfo.goodSeen:
			self.afterGoodCount += 1
		if agentInfo.badSeen:
			self.afterBadCount += 1

		if isgood:
			if agentInfo.goodSeen:
				self.goodAfterGood += 1
			else:
				agentInfo.goodSeen = True

			if agentInfo.badSeen:
				self.goodAfterBad += 1
		else:
			agentInfo.badSeen = True

		if firstEvent:
			agentInfo.foodSeen.add( food )
			return


		if isgood:
			self.goodIgnoreFirstCount += 1
		else:
			self.badIgnoreFirstCount += 1

		if food in agentInfo.foodSeen:
			return

		agentInfo.foodSeen.add( food )

		if len(agentInfo.foodSeen) == 2:
			if isgood:
				self.goodSecondPieceCount += 1
			else:
				self.badSecondPieceCount += 1

		if isgood:
			self.goodUniqueCount += 1
		else:
			self.badUniqueCount += 1

table = datalib.parse( run + '/events/energy.log', tablenames = ['Energy'] )[ 'Energy' ]

for row in table.rows():
	t = row['T']
	tepoch = (t / epochLen) * epochLen
	try:
		epoch = epochs[tepoch]
	except:
		epoch = Epoch(tepoch)
		epochs[tepoch] = epoch

	epoch.addEvent( row )

tepochs = sorted( epochs.keys() )
finalEpoch = epochs[ tepochs[-1] ]
if float(finalEpoch.finalTimestep - finalEpoch.timestep) / epochLen < 0.9:
	del tepochs[ -1 ]

colnames = ['T', 'AllEats', 'IgnoreFirstEat', 'IgnoreFirstEat_UniqueFood', 'SecondPiece', 'GoodAfterBad', 'GoodAfterGood']
coltypes = ['int', 'float', 'float', 'float', 'float', 'float', 'float']
table_fractionGood = datalib.Table( 'FractionGood', colnames, coltypes )

for tepoch in tepochs:
	epoch = epochs[tepoch]
	row = table_fractionGood.createRow()
	row['T'] = epoch.timestep
	row['AllEats'] = float(epoch.goodAllCount) / (epoch.goodAllCount + epoch.badAllCount)
	row['IgnoreFirstEat'] = float(epoch.goodIgnoreFirstCount) / (epoch.goodIgnoreFirstCount + epoch.badIgnoreFirstCount)
	row['IgnoreFirstEat_UniqueFood'] = float(epoch.goodUniqueCount) / (epoch.goodUniqueCount + epoch.badUniqueCount)
	if (epoch.goodSecondPieceCount + epoch.badSecondPieceCount) > 0:
		row['SecondPiece'] = float(epoch.goodSecondPieceCount) / (epoch.goodSecondPieceCount + epoch.badSecondPieceCount)
	else:
		row['SecondPiece'] = 0.0
	if epoch.afterBadCount > 0:
		row['GoodAfterBad'] = float(epoch.goodAfterBad) / epoch.afterBadCount
	else:
		row['GoodAfterBad'] = 0.0
	row['GoodAfterGood'] = float(epoch.goodAfterGood) / epoch.afterGoodCount


colnames = ['T', 'All', 'Good', 'Bad']
coltypes = ['int', 'int', 'int', 'int']
table_counts = datalib.Table( 'Counts', colnames, coltypes )

for tepoch in tepochs:
	epoch = epochs[tepoch]
	row = table_counts.createRow()
	row['T'] = epoch.timestep
	row['All'] = epoch.goodAllCount + epoch.badAllCount
	row['Good'] = epoch.goodAllCount
	row['Bad'] = epoch.badAllCount

colnames = ['T', 'GoodOverBad']
coltypes = ['int', 'float']
table_energy = datalib.Table( 'Energy', colnames, coltypes )

for tepoch in tepochs:
	epoch = epochs[tepoch]
	row = table_energy.createRow()
	row['T'] = epoch.timestep
	if epoch.badAllEnergy != 0:
		row['GoodOverBad'] = epoch.goodAllEnergy / epoch.badAllEnergy * -1
	else:
		row['GoodOverBad'] = epoch.goodAllEnergy

datalib.write( run + '/events/eatlearn.txt', [table_fractionGood, table_counts, table_energy] )
