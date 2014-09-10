void Simulate_Reference_to_Cache_Line(CDS *cds, memory_reference *reference)
{
    Boolean Found = FALSE;
    cache_line *cache_entry = NULL;
    cache_line *victim_cache_entry = NULL;
    int find_index;
    Boolean victim_cache_bool = FALSE;
    if (debug) fprintf(debug_file, "%s: %s Reference 0x%08X of length %d\n",
     cds->name, memory_reference_type_name(reference->type),
     reference->address, reference->length);
        
        cds->c->number_total_cache_access += 1;
    
    /* find cache line for this reference */
    /* find number of low-order bits to mask off to find beginning cache
       line address */
    
    memory_address cache_address = Base_Cache_Address(cds->c, reference->address);
    
    int cache_entry_index = Search_Cache_For(cds->c, cache_address);
    if (cache_entry_index >= 0)
    {
            /* found it -- record cache hit */
        Found = TRUE;
        if (debug) fprintf(debug_file, "%s: Found address 0x%08X in cache line %d\n", cds->name, 
         reference->address, cache_entry_index);
            cache_entry = &(cds->c->c_line[cache_entry_index]);
    }
    else
    {
            /* Did not find it. */
        Found = FALSE;
        
            /* Choose a victim from the set */
        cache_entry_index = Find_Victim_by_Replacement_Policy(cds->c, cache_address);
        cache_entry = &(cds->c->c_line[cache_entry_index]);
        if (debug) fprintf(debug_file, "%s: Pick victim %d to replace\n", cds->name,  cache_entry_index);
        
            /* evict victim */
        if (cache_entry->valid){
            if(cds->is_there_victim){
                evict_from_cache(cds, cache_entry, cache_address);
                find_index = Search_Cache_For(cds->v, cache_address);
                cds->v->number_total_cache_access +=1;
                if(find_index >=0){
                    victim_cache_entry = &(cds->v->c_line[find_index]);
                    victim_cache_bool = TRUE;
                    swap_cache_lines(victim_cache_entry,cache_entry);
                }else{
                        find_index = Find_Victim_by_Replacement_Policy(cds->v,cache_address); //Find victim in victim cache
                        victim_cache_entry = &(cds->v->c_line[find_index]);
                        if(victim_cache_entry->dirty){
                            evict_dirty_line_from_cache(cds->v, victim_cache_entry);
                        }
                        victim_cache_entry->valid = TRUE;
                        swap_cache_lines(victim_cache_entry,cache_entry);
                        cds->v->number_miss_reads+=1;
                    }
            Set_Replacement_Policy_Data(cds->number_of_memory_reference, cds->v, victim_cache_entry);
            }else{
                Found = evict_from_cache(cds, cache_entry, cache_address);
            }
            }
            if (!Found)
            {
                if(!victim_cache_bool){
                    /* fill in evicted cache line for this new line */
                
                cache_entry->valid = TRUE;
                cache_entry->tag = cache_address;
                cache_entry->dirty = FALSE;
                
                    /* read cache line from memory into cache table */
                if (debug) fprintf(debug_file, "%s: Read cache line 0x%08X into entry %d\n", cds->name,  cache_entry->tag, cache_entry_index);
                cds->c->number_miss_reads += 1;
                }
            }
        }
    /* update reference specific info */
        if (reference->type == MAT_STORE) 
        {
            /* If it's not write-back, then it is write-thru.
               For write-thru, if it's a write, we write to memory. */
            if (!cds->c->write_back)
            {
                cds->c->number_miss_writes += 1;
                if (debug) fprintf(debug_file, "%s: Write cache line 0x%08X thru to memory\n", cds->name,  cache_entry->tag);
            }
            else
                /* For write-back, if it's a write, it's dirty. */
                cache_entry->dirty = TRUE;
        }
        
        if (!Found)
            Set_Replacement_Policy_Data(cds->number_of_memory_reference, cds->c, cache_entry);
        else
            Update_Replacement_Policy_Data(cds->number_of_memory_reference, cds->c, cache_entry);
    }