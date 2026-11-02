"""Select semantic channel zero instead of concatenating cloth POSITION1 into geometry."""


def primary_vertex_channels(data, mesh, inflated):
    names={'POSITION':'positions','BONE_WEIGHTS':'bone_weights','BONE_INDICES':'bone_indices','UV':'uv'}
    counts={data.vertex_buffers[i].vertex_count for i in mesh.vertex_buffer_indices}
    if len(counts)!=1:raise ValueError('Vertex streams have different vertex counts')
    count=next(iter(counts)); selected={}; omitted=[]
    for index in mesh.vertex_buffer_indices:
        buffer=data.vertex_buffers[index]
        for member in data.vertex_buffer_structs[buffer.struct_index]:
            name=member.attribute_type.name
            if name not in names:continue
            if member.index!=0:
                omitted.append({'semantic':name,'index':member.index});continue
            if name in selected:raise ValueError('Duplicate primary vertex semantic: '+name)
            selected[name]=[member._unpack(buffer.buffer_data,i*buffer.struct_size,data.header.version) for i in range(count)]
    if 'POSITION' not in selected or 'UV' not in selected:raise ValueError('Primary position/UV channel missing')
    if any(i<0 or i>=count for face in inflated.faces for i in face):raise ValueError('Face references invalid primary vertex')
    for name,attribute in names.items():setattr(inflated.vertices,attribute,selected.get(name,[]))
    return {'vertex_count':count,'omitted_secondary_semantics':omitted}
