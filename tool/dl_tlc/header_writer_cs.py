HEADER_TEMPLATE = '''/*
	Autogenerated file for DL-type-library!
*/

using System;
using System.Runtime.InteropServices;

'''

PODS_ARRAY_TEMPLATE = '''	public %(cspod)s[] m_l%(name)s%(csdefault)s;
	private uint m_n%(name)sArraySize;'''

STR_ARRAY_TEMPLATE = '''	public %(subtype)s[] m_l%(name)s%(csdefault)s;
	private uint m_n%(name)sArraySize;'''

ENUM_ARRAY_TEMPLATE = '''	public E%(subtype)s[] m_l%(name)s%(csdefault)s;
	private uint m_n%(name)sArraySize;'''
	
ARRAY_TEMPLATE = '''	public C%(subtype)s[] m_l%(name)s%(csdefault)s;
	private uint m_n%(name)sArraySize;'''

BITFIELD_TEMPLATE = '''	public %(cspod)s m_%(name)s
	{
		get { %(cspod)s MASK = (1 << %(bits)u) - 1; return (%(cspod)s)((__BFStorage%(bf_storage_nr)d >> %(bfoffset)u) & MASK); }
		set { %(cspod)s MASK = (1 << %(bits)u) - 1; __BFStorage%(bf_storage_nr)d &= (%(cspod)s)~(MASK << %(bfoffset)u); __BFStorage%(bf_storage_nr)d |= (%(cspod)s)((MASK & value) << %(bfoffset)u); }
	}'''

PODS = [ 'int8', 'int16', 'int32', 'int64', 'uint8', 'uint16', 'uint32', 'uint64', 'fp32', 'fp64', 'string', 'bitfield' ]
CS_PODS = { 'int8'   : 'sbyte', 
			'int16'  : 'short', 
			'int32'  : 'int', 
			'int64'  : 'long', 
			'uint8'  : 'byte', 
			'uint16' : 'ushort', 
			'uint32' : 'uint', 
			'uint64' : 'ulong', 
			'fp32'   : 'float', 
			'fp64'   : 'double', 
			'string' : 'string', 
			'bitfield' : 'bitfield' }
	
def temp_hash_func(str):
	hash = 5381
	for char in str:
		hash = (hash * 33) + ord(char)
	return (hash - 5381) & 0xFFFFFFFF;
	
class HeaderWriterCS:
	def __write_header(self):
		print >> self.stream, HEADER_TEMPLATE % { 'module' : self.module }
	
	def __write_member(self, str, attribs):	
		if self.verbose:
			print >> self.stream, '\t// size %(size32)u, alignment %(align32)u, offset %(offset32)u' % attribs
			
		if 'comment' in attribs:
			print >> self.stream, (str + '\t// %(comment)s') % attribs
		else:
			print >> self.stream, str % attribs
		
	def write_enums(self, data):
		if data.has_key('module_enums'):
			for enum in data['module_enums'].items():
				enum_name = enum[0]
				enum_base = enum_name.upper()
				enum_values = enum[1]
				
				self.stream.write('public enum E' + enum_name + ' : uint\n{')
				for i in range(0, len(enum_values)):
					value = enum_values[i]
					self.stream.write('\n\t%s_%s = %d' % (enum_base, value[0].upper(), value[1]))
					
					if i != len(enum_values) - 1:
						self.stream.write(',')
				print >> self.stream, '\n};\n'
		
	def create_header_order(self, outlist, type, data, enums):
		if type in outlist or type in PODS or type in enums:
			return
		
		member_types = []
		for m in data[type]['members']:
			m_type = m.get('subtype', m['type'])
			if m_type not in member_types and m_type != type:
				member_types.append(m_type)
		
		for m in member_types:
			self.create_header_order(outlist, m, data, enums)
			
		outlist.append(type)
	
	def write_structs(self, data):
		write_order = []
		
		mod_types  = data['module_types']
		enum_types = data.get('module_enums', {}).keys()
		
		for struct in mod_types.items():
			self.create_header_order(write_order, struct[0], mod_types, enum_types)
		
		for type_name in write_order:
			struct_name   = type_name
			struct_attrib = mod_types[type_name]
			
			if 'cs-alias' in struct_attrib: continue # we have an alias so we force the user to provide the type.
			
			print >> self.stream, '// size ', struct_attrib['size32']
			if 'comment' in struct_attrib: print >> self.stream, '//', struct_attrib['comment']
			print >> self.stream, '[StructLayout(LayoutKind.Sequential, Size=%s, CharSet=CharSet.Ansi)]\npublic class C%s\n{' % (struct_attrib['size32'], struct_name)
			print >> self.stream, '\tpublic const UInt32 TYPE_ID = 0x%08X;\n' % temp_hash_func(struct_name)
			
			# need to generate constructor here to be able to do default-sturcts
			
			bf_storage_nr   = 0
			for member in struct_attrib['members']:
				type = member['type']

				if type in CS_PODS:
					member['cspod'] = CS_PODS[type]
				
				if 'subtype' in member and member['subtype'] in CS_PODS:
					member['cspod'] = CS_PODS[member['subtype']]
					
				if 'default' in member:
					if   type == 'inline-array' or type == 'array':
						def_str = ''
						if   member['subtype'] == 'string': def_str = '{ ' + ', '.join( [ '"%s"' % elem for elem in member['default'] ] ) + ' }'
						elif member['subtype'] == 'fp32':   def_str = '{ ' + ', '.join( [ '%ff'  % elem for elem in member['default'] ] ) + ' }'
						elif member['subtype'] == 'fp64':   def_str = '{ ' + ', '.join( [ '%f'   % elem for elem in member['default'] ] ) + ' }'
						elif member['subtype'] in PODS:     def_str = '{ ' + ', '.join( [ '%u'   % elem for elem in member['default'] ] ) + ' }'
						elif member['subtype'] in data['module_enums']:	
							sub_upper = member['subtype'].upper()
							def_str = '{ ' + ', '.join( [ sub_upper + '_%s'   % elem.upper() for elem in member['default'] ] ) + ' }'
						
						member['csdefault'] = ' = ' + def_str % member
						
						# skip structs, how to handle these?
						
					elif type == 'string':       member['csdefault'] = ' = "%(default)s"' % member
					elif type == 'bitfield':     member['csdefault'] = ''
					elif type == 'fp32':         member['csdefault'] = ' = %(default)ff' % member
					elif type == 'fp64':         member['csdefault'] = ' = %(default)f' % member
					elif type in PODS:           member['csdefault'] = ' = %(default)d' % member
					elif type in enum_types:     member['csdefault'] = ' = %s_%s' % (type.upper(), member['default'].upper())
					# skip structs
				else:
					member['csdefault'] = ''
				
				if type == 'inline-array':
					if   member['subtype'] == 'string':   self.__write_member('\t[MarshalAsAttribute(UnmanagedType.ByValTStr,  SizeConst=%(count)u)]\n\tpublic string[] m_lp%(name)s%(csdefault)s;',      member)
					elif member['subtype'] in PODS:       self.__write_member('\t[MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst=%(count)u)]\n\tpublic %(cspod)s[] m_l%(name)s%(csdefault)s;',    member)
					elif member['subtype'] in enum_types: self.__write_member('\t[MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst=%(count)u)]\n\tpublic E%(subtype)s[] m_l%(name)s;', member)
					else:                                 self.__write_member('\t[MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst=%(count)u)]\n\tpublic C%(subtype)s[] m_l%(name)s;', member)
				elif type == 'array':
					if   member['subtype'] == 'string':   self.__write_member(STR_ARRAY_TEMPLATE,  member)
					elif member['subtype'] in PODS:       self.__write_member(PODS_ARRAY_TEMPLATE, member)
					elif member['subtype'] in enum_types: self.__write_member(ENUM_ARRAY_TEMPLATE, member)
					else:                                 self.__write_member(ARRAY_TEMPLATE,      member)
				elif type == 'string':  self.__write_member('\tpublic string m_p%(name)s%(csdefault)s;', member)
				elif type == 'pointer': self.__write_member('\tpublic C%(subtype)s m_p%(name)s = null;', member)
				elif type == 'bitfield': 
					member['bf_storage_nr'] = bf_storage_nr
					self.__write_member(BITFIELD_TEMPLATE, member)
					if member['last_in_bf']:
						self.__write_member('\tprivate %(cspod)s __BFStorage%(bf_storage_nr)d;', member)
						bf_storage_nr += 1
				elif type in PODS:                  self.__write_member('\tpublic %(cspod)s m_%(name)s%(csdefault)s;', member)
				elif type in enum_types:            self.__write_member('\tpublic E%(type)s m_%(name)s%(csdefault)s;', member)
				elif 'cs-alias' in mod_types[type]: self.__write_member('\t%s m_%%(name)s;' % mod_types[type]['cs-alias'], member)
				else:                               self.__write_member('\tpublic C%(type)s m_%(name)s;', member)
			
			print >> self.stream, '};\n'
		
	def __init__(self, out, module):
		self.stream  = out
		self.module  = module
		self.verbose = False
		
		self.__write_header()
