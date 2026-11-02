import unittest
from types import SimpleNamespace as NS
from flver_layout import primary_vertex_channels


class LayoutTests(unittest.TestCase):
    def member(self,name,index,values):
        return NS(attribute_type=NS(name=name),index=index,_unpack=lambda data,offset,version:values[offset])

    def test_secondary_cloth_positions_are_not_new_vertices(self):
        members=[self.member('POSITION',0,[(1,2,3),(4,5,6)]),self.member('POSITION',1,[(9,9,9),(8,8,8)]),
                 self.member('UV',0,[(0,0),(1,1)]),self.member('BONE_WEIGHTS',0,[(1,0,0,0)]*2),
                 self.member('BONE_INDICES',0,[(5,0,0,0)]*2)]
        data=NS(vertex_buffers=[NS(vertex_count=2,struct_index=0,buffer_data=b'',struct_size=1)],
                vertex_buffer_structs=[members],header=NS(version=1))
        inflated=NS(vertices=NS(),faces=[(0,1,0)])
        result=primary_vertex_channels(data,NS(vertex_buffer_indices=[0]),inflated)
        self.assertEqual(inflated.vertices.positions,[(1,2,3),(4,5,6)])
        self.assertEqual(len(inflated.vertices.bone_weights),2)
        self.assertEqual(result['omitted_secondary_semantics'],[{'semantic':'POSITION','index':1}])
        members.append(self.member('POSITION',0,[(0,0,0)]*2))
        with self.assertRaises(ValueError):primary_vertex_channels(data,NS(vertex_buffer_indices=[0]),inflated)


if __name__=='__main__':unittest.main()
